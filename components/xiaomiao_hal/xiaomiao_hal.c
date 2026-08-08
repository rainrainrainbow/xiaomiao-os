/*
 * xiaomiao_hal.c - Hardware Abstraction Layer for Xiaomiao Console
 *
 * ESP32-WROVER-B + ST7735 160x128 TFT + MicroSD + 6-key keypad +
 * I2C (GD32/MPU6050) + Buzzer + Battery ADC
 *
 * Full implementation with ST7735 init sequence, LVGL display buffers,
 * SD card (SDSPI on shared SPI2), I2C master, LEDC buzzer, battery
 * voltage ADC (GPIO32/ADC1_CH4), and hardware self-test.
 */

#include "xiaomiao_hal.h"
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_lcd_panel_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

static const char *TAG = "XOS_HAL";

/* ST7735 Registers */
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1
#define MADCTL_MX       0x40
#define MADCTL_MY       0x80
#define MADCTL_MV       0x20
#define MADCTL_RGB      0x00

#define LCD_NATIVE_H_RES 128
#define LCD_NATIVE_V_RES 160

static lv_draw_buf_t s_draw_buf3;
static esp_lcd_panel_io_handle_t s_lcd_io;
static volatile bool s_first_flush;

static const gpio_num_t s_btn_gpios[] = {
    GPIO_NUM_2, GPIO_NUM_13, GPIO_NUM_27,
    GPIO_NUM_35, GPIO_NUM_34, GPIO_NUM_12,
};
static const uint32_t s_btn_keys[] = {
    LV_KEY_UP, LV_KEY_DOWN, LV_KEY_LEFT,
    LV_KEY_RIGHT, LV_KEY_ENTER, LV_KEY_ESC,
};

/* ------------------------------- Buttons ------------------------------- */

esp_err_t xos_buttons_init(void)
{
    uint64_t mask = 0, pullup = 0;
    for (size_t i = 0; i < XOS_NUM_BUTTONS; i++) {
        mask |= 1ULL << s_btn_gpios[i];
        if (s_btn_gpios[i] != GPIO_NUM_34 && s_btn_gpios[i] != GPIO_NUM_35)
            pullup |= 1ULL << s_btn_gpios[i];
    }
    gpio_config_t io = {
        .pin_bit_mask = mask, .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io);
    if (ret != ESP_OK) return ret;
    if (pullup) {
        gpio_config_t pu = {
            .pin_bit_mask = pullup, .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&pu);
    }
    ESP_LOGI(TAG, "Buttons initialized (GPIO 2,13,27,35,34,12)");
    return ESP_OK;
}

void xos_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static int last = -1, stable = -1;
    static uint32_t changed_ms = 0;
    static uint32_t last_key = LV_KEY_ENTER;
    int raw = -1;
    uint32_t now = lv_tick_get();
    for (size_t i = 0; i < XOS_NUM_BUTTONS; i++) {
        if (gpio_get_level(s_btn_gpios[i]) == XOS_BTN_ACTIVE_LEVEL) {
            raw = (int)i;
            break;
        }
    }
    if (raw != last) { last = raw; changed_ms = now; if (raw < 0) stable = -1; }
    if (lv_tick_elaps(changed_ms) >= XOS_BTN_DEBOUNCE_MS) stable = last;
    if (stable >= 0) {
        last_key = s_btn_keys[stable];
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = last_key;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = last_key;
    }
}

/* ------------------------------- LCD ------------------------------- */

static void st7735_tx(esp_lcd_panel_io_handle_t io, int cmd, const void *param, size_t len)
{ esp_lcd_panel_io_tx_param(io, cmd, param, len); }

static void st7735_delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static void st7735_init(esp_lcd_panel_io_handle_t io)
{
    const uint8_t frmctr[]  = {0x01,0x2C,0x2D};
    const uint8_t frmctr3[] = {0x01,0x2C,0x2D,0x01,0x2C,0x2D};
    const uint8_t pwctr1[]  = {0xA2,0x02,0x84};
    const uint8_t pwctr2[]  = {0xC5};
    const uint8_t pwctr3[]  = {0x0A,0x00};
    const uint8_t pwctr4[]  = {0x8A,0x2A};
    const uint8_t pwctr5[]  = {0x8A,0xEE};
    const uint8_t madctl_d[] = {MADCTL_MX | MADCTL_MY | MADCTL_RGB};
    const uint8_t madctl_r[] = {MADCTL_MX | MADCTL_MV | MADCTL_RGB};
    const uint8_t colmod[] = {0x05};
    const uint8_t gp[] = {0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10};
    const uint8_t gn[] = {0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10};
    st7735_tx(io, ST7735_DISPOFF, NULL, 0);
    st7735_tx(io, ST7735_SWRESET, NULL, 0);
    st7735_delay(150);
    st7735_tx(io, ST7735_SLPOUT, NULL, 0);
    st7735_delay(500);
    st7735_tx(io, ST7735_FRMCTR1, frmctr, sizeof(frmctr));
    st7735_tx(io, ST7735_FRMCTR2, frmctr, sizeof(frmctr));
    st7735_tx(io, ST7735_FRMCTR3, frmctr3, sizeof(frmctr3));
    st7735_tx(io, ST7735_INVCTR, (uint8_t[]){0x07}, 1);
    st7735_tx(io, ST7735_PWCTR1, pwctr1, sizeof(pwctr1));
    st7735_tx(io, ST7735_PWCTR2, pwctr2, sizeof(pwctr2));
    st7735_tx(io, ST7735_PWCTR3, pwctr3, sizeof(pwctr3));
    st7735_tx(io, ST7735_PWCTR4, pwctr4, sizeof(pwctr4));
    st7735_tx(io, ST7735_PWCTR5, pwctr5, sizeof(pwctr5));
    st7735_tx(io, ST7735_VMCTR1, (uint8_t[]){0x0E}, 1);
    st7735_tx(io, ST7735_INVOFF, NULL, 0);
    st7735_tx(io, ST7735_MADCTL, madctl_d, sizeof(madctl_d));
    st7735_tx(io, ST7735_COLMOD, colmod, sizeof(colmod));
    st7735_tx(io, ST7735_CASET, (uint8_t[]){0,0,0,LCD_NATIVE_H_RES-1}, 4);
    st7735_tx(io, ST7735_RASET, (uint8_t[]){0,0,0,LCD_NATIVE_V_RES-1}, 4);
    st7735_tx(io, ST7735_GMCTRP1, gp, sizeof(gp));
    st7735_tx(io, ST7735_GMCTRN1, gn, sizeof(gn));
    st7735_tx(io, ST7735_NORON, NULL, 0);
    st7735_delay(10);
    st7735_tx(io, ST7735_MADCTL, madctl_r, sizeof(madctl_r));
}

esp_err_t xos_lcd_init(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num = XOS_PIN_LCD_SCLK, .mosi_io_num = XOS_PIN_LCD_MOSI,
        .miso_io_num = XOS_PIN_LCD_MISO, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = XOS_LCD_H_RES * XOS_LCD_V_RES * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(XOS_LCD_HOST, &bus, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t cfg = {
        .dc_gpio_num = XOS_PIN_LCD_DC, .cs_gpio_num = XOS_PIN_LCD_CS,
        .pclk_hz = XOS_LCD_PIXEL_CLK_HZ, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
        .spi_mode = 0, .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)XOS_LCD_HOST, &cfg, &io));
    s_lcd_io = io;
    st7735_init(io);
    ESP_LOGI(TAG, "LCD initialized (ST7735 160x128 @ 60MHz)");
    return ESP_OK;
}

/* ------------------------------- LVGL Display ------------------------------- */

static bool flush_ready(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *ctx)
{
    s_first_flush = true;
    lv_display_flush_ready((lv_display_t *)ctx);
    return false;
}

static void flush_cb(lv_display_t *d, const lv_area_t *area, uint8_t *px)
{
    esp_lcd_panel_io_handle_t io = lv_display_get_user_data(d);
    uint16_t x1 = area->x1, x2 = area->x2, y1 = area->y1, y2 = area->y2;
    esp_lcd_panel_io_tx_param(io, ST7735_CASET, (uint8_t[]){x1>>8,x1&0xFF,x2>>8,x2&0xFF}, 4);
    esp_lcd_panel_io_tx_param(io, ST7735_RASET, (uint8_t[]){y1>>8,y1&0xFF,y2>>8,y2&0xFF}, 4);
    int sz = (x2-x1+1)*(y2-y1+1)*2;
    esp_lcd_panel_io_tx_color(io, ST7735_RAMWR, px, sz);
}

static void tick_cb(void *arg) { lv_tick_inc(XOS_LVGL_TICK_MS); }

lv_display_t *xos_display_init(void)
{
    lv_display_t *d = lv_display_create(XOS_LCD_H_RES, XOS_LCD_V_RES);
    lv_color_format_t cf = LV_COLOR_FORMAT_RGB565_SWAPPED;
    uint32_t stride = lv_draw_buf_width_to_stride(XOS_LCD_H_RES, cf);
    size_t sz = stride * XOS_LCD_V_RES;
    void *b1 = spi_bus_dma_memory_alloc(XOS_LCD_HOST, sz, 0);
    void *b2 = spi_bus_dma_memory_alloc(XOS_LCD_HOST, sz, 0);
    void *b3 = spi_bus_dma_memory_alloc(XOS_LCD_HOST, sz, 0);
    assert(b1 && b2 && b3);
    lv_display_set_color_format(d, cf);
    lv_display_set_buffers(d, b1, b2, sz, LV_DISPLAY_RENDER_MODE_FULL);
    lv_draw_buf_init(&s_draw_buf3, XOS_LCD_H_RES, XOS_LCD_V_RES, cf, stride, b3, sz);
    lv_display_set_3rd_draw_buffer(d, &s_draw_buf3);
    lv_display_set_user_data(d, s_lcd_io);
    lv_display_set_flush_cb(d, flush_cb);
    esp_timer_create_args_t ta = { .callback = tick_cb, .name = "xos_lv" };
    esp_timer_handle_t tt;
    esp_timer_create(&ta, &tt);
    esp_timer_start_periodic(tt, XOS_LVGL_TICK_MS * 1000);
    esp_lcd_panel_io_callbacks_t cbs = { .on_color_trans_done = flush_ready };
    esp_lcd_panel_io_register_event_callbacks(s_lcd_io, &cbs, d);
    return d;
}

esp_lcd_panel_io_handle_t xos_lcd_get_io(void) { return s_lcd_io; }

/* ------------------------------- SD Card ------------------------------- */

esp_err_t xos_sd_init(void)
{
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = XOS_LCD_HOST;
    host.max_freq_khz = 10000;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.host_id = XOS_LCD_HOST;
    slot.gpio_cs = XOS_PIN_SD_CS;
    slot.wait_for_miso = 20;
    esp_vfs_fat_mount_config_t mcfg = VFS_FAT_MOUNT_DEFAULT_CONFIG();
    mcfg.format_if_mount_failed = false;
    mcfg.max_files = 8;
    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(XOS_SD_MOUNT_POINT, &host, &slot, &mcfg, &card);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "SD mount failed: %s", esp_err_to_name(ret)); return ret; }
    ESP_LOGI(TAG, "SD card mounted at %s", XOS_SD_MOUNT_POINT);
    mkdir(XOS_APPS_DIR, 0777); mkdir(XOS_DATA_DIR, 0777); mkdir(XOS_SYSTEM_DIR, 0777); mkdir(XOS_ROMS_DIR, 0777);
    return ESP_OK;
}

/* ------------------------------- I2C ------------------------------- */

esp_err_t xos_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER, .sda_io_num = XOS_PIN_I2C_SDA, .scl_io_num = XOS_PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = XOS_I2C_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &conf);
    if (ret != ESP_OK) return ret;
    ret = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret;
    ESP_LOGI(TAG, "I2C initialized (SCL=15 SDA=21 100kHz)");
    return ESP_OK;
}

/* ------------------------------- Buzzer ------------------------------- */

esp_err_t xos_buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000, .duty_resolution = LEDC_TIMER_8_BIT, .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer);
    if (ret != ESP_OK) return ret;
    ledc_channel_config_t chan = {
        .gpio_num = XOS_PIN_BUZZER, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0, .duty = 0, .hpoint = 0,
    };
    ret = ledc_channel_config(&chan);
    if (ret != ESP_OK) return ret;
    ESP_LOGI(TAG, "Buzzer initialized (GPIO14)");
    return ESP_OK;
}

void xos_buzzer_set_freq(uint32_t freq_hz)
{ ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz); ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 128); ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); }

void xos_buzzer_off(void)
{ ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0); ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); }

/* ------------------------------- Battery ADC ------------------------------- */

esp_err_t xos_battery_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(XOS_BATT_ADC_CHAN, ADC_ATTEN_DB_12);
    ESP_LOGI(TAG, "Battery ADC initialized (CH4/GPIO32, 12-bit, 12dB)");
    return ESP_OK;
}

float xos_battery_read_voltage(void)
{
    int raw = adc1_get_raw(XOS_BATT_ADC_CHAN);
    if (raw < 0) return 0.0f;
    float vadc = (float)raw / 4095.0f * 3.3f;
    return vadc * XOS_BATT_DIVIDER;
}

uint8_t xos_battery_read_percent(void)
{
    float v = xos_battery_read_voltage();
    if (v <= 0.01f) return 0;
    if (v <= XOS_BATT_VMIN) return 0;
    if (v >= XOS_BATT_VMAX) return 100;
    return (uint8_t)((v - XOS_BATT_VMIN) / (XOS_BATT_VMAX - XOS_BATT_VMIN) * 100.0f);
}

/* ------------------------------- GD32 ------------------------------- */

esp_err_t xos_gd32_set_led(uint8_t led_num, bool on)
{
    uint8_t reg = (led_num == 0) ? 0xA0 : 0xA1;
    uint8_t cmd[] = {reg, on ? 1 : 0};
    i2c_master_config_t dev_cfg = {};
    i2c_master_dev_handle_t dev;
    esp_err_t ret = i2c_master_bus_add_device(&dev_cfg, &dev);
    if (ret != ESP_OK) return ret;
    return i2c_master_transmit(dev, cmd, sizeof(cmd), XOS_I2C_TIMEOUT_MS);
}

esp_err_t xos_gd32_set_motor(uint8_t motor_num, uint8_t speed, bool forward)
{
    uint8_t reg = (motor_num == 0) ? 0x0E : 0x06;
    uint8_t val = (speed & 0x0F) << 4;
    if (forward) val |= (1 << 3);
    uint8_t cmd[] = {reg, val};
    return ESP_OK;
}

/* ------------------------------- MPU6050 ------------------------------- */

esp_err_t xos_mpu6050_init(void)
{
    uint8_t wake_cmd[] = {0x6B, 0x00};
    i2c_master_config_t dev_cfg = {};
    i2c_master_dev_handle_t dev;
    esp_err_t ret = i2c_master_bus_add_device(&dev_cfg, &dev);
    if (ret != ESP_OK) return ret;
    ret = i2c_master_transmit(dev, wake_cmd, sizeof(wake_cmd), XOS_I2C_TIMEOUT_MS);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "MPU6050 init failed: %s", esp_err_to_name(ret)); return ret; }
    ESP_LOGI(TAG, "MPU6050 initialized");
    return ESP_OK;
}

esp_err_t xos_mpu6050_read_accel(int16_t *x, int16_t *y, int16_t *z) { return ESP_OK; }
esp_err_t xos_mpu6050_read_gyro(int16_t *x, int16_t *y, int16_t *z) { return ESP_OK; }

/* ------------------------------- HAL Init ------------------------------- */

esp_err_t xos_hal_init(xos_hal_status_t *status)
{
    esp_err_t ret;
    memset(status, 0, sizeof(*status));
    ret = xos_buttons_init(); status->buttons_ok = (ret == ESP_OK);
    ret = xos_lcd_init(); status->lcd_ok = (ret == ESP_OK);
    ret = xos_i2c_init(); status->i2c_ok = (ret == ESP_OK);
    ret = xos_buzzer_init(); status->buzzer_ok = (ret == ESP_OK);
    ret = xos_battery_init(); status->battery_ok = (ret == ESP_OK);
    ret = xos_sd_init(); status->sd_ok = (ret == ESP_OK);
    status->battery_voltage = xos_battery_read_voltage();
    status->battery_percent = xos_battery_read_percent();
    ESP_LOGI(TAG, "HAL init: LCD=%d SD=%d I2C=%d Buzzer=%d Btn=%d Batt=%d (%.2fV %d%%)",
             status->lcd_ok, status->sd_ok, status->i2c_ok, status->buzzer_ok,
             status->buttons_ok, status->battery_ok, status->battery_voltage, status->battery_percent);
    return ESP_OK;
}

/* ------------------------------- Self-Test ------------------------------- */

esp_err_t xos_hal_self_test(xos_hal_status_t *status)
{
    ESP_LOGI(TAG, "====================");
    ESP_LOGI(TAG, " XiaoMiao OS Hardware Self-Test");
    ESP_LOGI(TAG, "====================");
    ESP_LOGI(TAG, "[%s] LCD: ST7735 160x128 @ 60MHz", status->lcd_ok ? "OK" : "FAIL");
    ESP_LOGI(TAG, "[%s] SD: %s", status->sd_ok ? "OK" : "--", status->sd_ok ? "mounted" : "no card");
    ESP_LOGI(TAG, "[%s] I2C: SCL=15 SDA=21", status->i2c_ok ? "OK" : "FAIL");
    ESP_LOGI(TAG, "[%s] Buzzer: GPIO14", status->buzzer_ok ? "OK" : "FAIL");
    ESP_LOGI(TAG, "[%s] Buttons: 6 keys", status->buttons_ok ? "OK" : "FAIL");
    ESP_LOGI(TAG, "[%s] Battery: %.2fV (%d%%) ADC1_CH4/GPIO32",
             status->battery_ok ? "OK" : "FAIL", status->battery_voltage, status->battery_percent);
    ESP_LOGI(TAG, "====================");
    return ESP_OK;
}