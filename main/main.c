/*
 * Xiaomiao OS — Desktop System for Xiaomiao Education Console
 * ESP32-WROVER-B + ST7735 160x128 TFT + MicroSD + 6-key + GD32
 * Hardware Verification & HAL Layer
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "driver/adc_types_legacy.h"
#include "esp_lcd_panel_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"
#include "return_to_loader.h"
#include "app_manager.h"
#include "app_store.h"
#include "power.h"
#include "micropython_runtime.h"
#include "ui/ui_main.h"

/* ── Hardware Constants ────────────────────────────────────────────── */

/* ST7735 LCD (SPI2 @ 60MHz) */
#define LCD_HOST            SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ  (60 * 1000 * 1000)
#define LCD_NATIVE_H_RES    128
#define LCD_NATIVE_V_RES    160
#define LCD_H_RES           160
#define LCD_V_RES           128
#define PIN_LCD_SCLK        GPIO_NUM_18
#define PIN_LCD_MOSI        GPIO_NUM_23
#define PIN_LCD_MISO        GPIO_NUM_19
#define PIN_LCD_CS          GPIO_NUM_5
#define PIN_LCD_DC          GPIO_NUM_4

/* Backlight (GPIO0 PWM) */
#define PIN_BL              GPIO_NUM_0

/* SD Card (SPI2, CS=22) */
#define PIN_SD_CS           GPIO_NUM_22
#define SD_MAX_FREQ_KHZ     10000

/* Buttons (active-low) */
#define PIN_BTN_UP          GPIO_NUM_2
#define PIN_BTN_DOWN        GPIO_NUM_13
#define PIN_BTN_LEFT        GPIO_NUM_27
#define PIN_BTN_RIGHT       GPIO_NUM_35  /* input-only, no pull-up */
#define PIN_BTN_A           GPIO_NUM_34  /* input-only, also ADC for battery */
#define PIN_BTN_B           GPIO_NUM_12
#define BUTTON_ACTIVE_LEVEL 0
#define BUTTON_DEBOUNCE_MS  25

/* I2C (GD32 + MPU6050) */
#define I2C_PORT            I2C_NUM_0
#define I2C_SCL             GPIO_NUM_15
#define I2C_SDA             GPIO_NUM_21
#define I2C_FREQ_HZ         100000
#define I2C_TIMEOUT_MS      30
#define GD32_ADDR           0x40
#define GD32_REG_LED1       0xA0
#define GD32_REG_LED2       0xA1
#define GD32_REG_MOTOR1     0x0E
#define GD32_REG_MOTOR2     0x06
#define MPU6050_ADDR        0x68

/* Buzzer (GPIO14, LEDC) */
#define PIN_BUZZ            GPIO_NUM_14
#define BUZZ_TIMER          LEDC_TIMER_0
#define BUZZ_CHANNEL        LEDC_CHANNEL_0
#define BUZZ_MODE           LEDC_LOW_SPEED_MODE

/* LVGL */
#define LVGL_TICK_PERIOD_MS 1

/* UI Colors */
#define UI_YELLOW           0xF6D34A
#define UI_BLACK            0x1B1713
#define UI_BROWN            0x5C4220
#define UI_RED              0xE64B3C
#define UI_CREAM            0xFFF3B0
#define UI_GREEN            0x2DD466

/* ST7735 commands */
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

static const char *TAG = "XiaoMiaoOS";

/* ── Globals ───────────────────────────────────────────────────────── */

static lv_draw_buf_t          s_draw_buf3;
static esp_lcd_panel_io_handle_t s_lcd_io;
static volatile bool          s_first_flush = false;
static bool                   s_sd_mounted = false;
static float                  s_battery_voltage = 0.0f;
static sdmmc_card_t          *s_card = NULL;

/* New UI system globals */
static app_registry_t         s_app_registry;
static ui_manager_t           s_ui_manager;

/* Power management task handle */
static TaskHandle_t           s_power_task_handle = NULL;

/* ── Button Map ────────────────────────────────────────────────────── */

static const gpio_num_t s_btn_gpios[] = {
    PIN_BTN_UP,   PIN_BTN_DOWN, PIN_BTN_LEFT,
    PIN_BTN_RIGHT, PIN_BTN_A,   PIN_BTN_B,
};
static const uint32_t s_btn_keys[] = {
    LV_KEY_UP, LV_KEY_DOWN, LV_KEY_LEFT,
    LV_KEY_RIGHT, LV_KEY_ENTER, LV_KEY_ESC,
};
#define NUM_BUTTONS (sizeof(s_btn_gpios) / sizeof(s_btn_gpios[0]))

/* ── Backlight Control (GPIO0 PWM) ─────────────────────────────────── */

static void backlight_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 5000,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t chan = {
        .gpio_num = PIN_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_1,
        .duty = 255,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan));
    ESP_LOGI(TAG, "Backlight: GPIO0 PWM 5kHz");
}

static void backlight_set(uint8_t brightness)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

/* ── Buttons ───────────────────────────────────────────────────────── */

static void buttons_init(void)
{
    uint64_t mask = 0, pullup = 0;
    for (size_t i = 0; i < NUM_BUTTONS; i++) {
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
    gpio_config(&io);
    if (pullup) {
        gpio_config_t pu = {
            .pin_bit_mask = pullup, .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&pu);
    }
    ESP_LOGI(TAG, "Buttons: 6-key active-low");
}

static void keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static int last = -1, stable = -1;
    static uint32_t changed_ms = 0;
    static uint32_t last_key = LV_KEY_ENTER;
    static bool key_dispatched = false;
    int raw = -1;
    uint32_t now = lv_tick_get();

    for (size_t i = 0; i < NUM_BUTTONS; i++) {
        if (gpio_get_level(s_btn_gpios[i]) == BUTTON_ACTIVE_LEVEL) {
            raw = (int)i;
            break;
        }
    }
    if (raw != last) {
        last = raw;
        changed_ms = now;
        if (raw < 0) stable = -1;
    }
    if (lv_tick_elaps(changed_ms) >= BUTTON_DEBOUNCE_MS)
        stable = last;

    if (stable >= 0) {
        last_key = s_btn_keys[stable];
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = last_key;
        
        /* Dispatch key press to UI manager (only once per press) */
        if (!key_dispatched) {
            ui_manager_handle_key(&s_ui_manager, last_key, true);
            key_dispatched = true;
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = last_key;
        key_dispatched = false;  /* Reset for next press */
    }
}

/* ── ST7735 LCD ────────────────────────────────────────────────────── */

static void st7735_tx(esp_lcd_panel_io_handle_t io, int cmd,
                      const void *param, size_t len)
{
    esp_lcd_panel_io_tx_param(io, cmd, param, len);
}

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
    const uint8_t colmod[]  = {0x05};
    const uint8_t gp[] = {0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
                          0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10};
    const uint8_t gn[] = {0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
                          0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10};

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
    st7735_tx(io, ST7735_CASET,
              (uint8_t[]){0,0,0,LCD_NATIVE_H_RES-1}, 4);
    st7735_tx(io, ST7735_RASET,
              (uint8_t[]){0,0,0,LCD_NATIVE_V_RES-1}, 4);
    st7735_tx(io, ST7735_GMCTRP1, gp, sizeof(gp));
    st7735_tx(io, ST7735_GMCTRN1, gn, sizeof(gn));
    st7735_tx(io, ST7735_NORON, NULL, 0);
    st7735_delay(10);
    st7735_tx(io, ST7735_MADCTL, madctl_r, sizeof(madctl_r));
}

static esp_lcd_panel_io_handle_t lcd_init(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = PIN_LCD_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI2 bus: 60MHz, shared LCD+SD");

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t cfg = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST, &cfg, &io));
    s_lcd_io = io;
    st7735_init(io);
    ESP_LOGI(TAG, "ST7735 LCD: 160x128 RGB565");
    return io;
}

/* ── SD Card ───────────────────────────────────────────────────────── */

static esp_err_t sd_card_init(void)
{
    esp_vfs_fat_mount_config_t mcfg = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = SD_MAX_FREQ_KHZ;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.host_id = SPI2_HOST;
    slot.gpio_cs = PIN_SD_CS;
    slot.gpio_miso = GPIO_NUM_19;
    slot.gpio_mosi = GPIO_NUM_23;
    slot.gpio_sck  = GPIO_NUM_18;
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot,
                                            &mcfg, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        s_sd_mounted = false;
        return ret;
    }
    s_sd_mounted = true;
    ESP_LOGI(TAG, "SD card: /sdcard, %llu MB",
        (uint64_t)(s_card->csd.capacity) * s_card->csd.sector_size / (1024*1024));
    return ESP_OK;
}

/* ── SPIFFS (VFS partition) ────────────────────────────────────────── */

static esp_err_t spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/vfs",
        .partition_label = "vfs",
        .max_files = 8,
        .format_if_mount_failed = true,
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }
    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);
    ESP_LOGI(TAG, "SPIFFS: /vfs, total=%dKB used=%dKB", total/1024, used/1024);
    return ESP_OK;
}

/* ── I2C ────────────────────────────────────────────────────────────── */

static void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    /* Probe GD32 and MPU6050 */
    esp_err_t gd = i2c_master_write_to_device(I2C_PORT, GD32_ADDR, NULL, 0, I2C_TIMEOUT_MS);
    esp_err_t mpu = i2c_master_write_to_device(I2C_PORT, MPU6050_ADDR, NULL, 0, I2C_TIMEOUT_MS);
    ESP_LOGI(TAG, "I2C: GD32=%s MPU6050=%s",
        gd == ESP_OK ? "OK" : "FAIL",
        mpu == ESP_OK ? "OK" : "FAIL");

    /* Turn on LED1 as power indicator via GD32 */
    if (gd == ESP_OK) {
        uint8_t buf1[] = {GD32_REG_LED1, 1};
        i2c_master_write_to_device(I2C_PORT, GD32_ADDR, buf1, 2, I2C_TIMEOUT_MS);
    }
}

/* ── Buzzer ──────────────────────────────────────────────────────────── */

static void buzzer_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = BUZZ_MODE,
        .timer_num = BUZZ_TIMER,
        .freq_hz = 988,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t chan = {
        .gpio_num = PIN_BUZZ,
        .speed_mode = BUZZ_MODE,
        .channel = BUZZ_CHANNEL,
        .timer_sel = BUZZ_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan));
    ESP_LOGI(TAG, "Buzzer: GPIO14 LEDC");
}

static void buzzer_beep(uint16_t freq, uint8_t duty, uint32_t ms)
{
    ledc_set_freq(BUZZ_MODE, BUZZ_TIMER, freq);
    ledc_set_duty(BUZZ_MODE, BUZZ_CHANNEL, duty);
    ledc_update_duty(BUZZ_MODE, BUZZ_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(ms));
    ledc_set_duty(BUZZ_MODE, BUZZ_CHANNEL, 0);
    ledc_update_duty(BUZZ_MODE, BUZZ_CHANNEL);
}

/* ── LVGL Display ────────────────────────────────────────────────────── */

static bool flush_ready(esp_lcd_panel_io_handle_t io,
                        esp_lcd_panel_io_event_data_t *edata, void *ctx)
{
    s_first_flush = true;
    lv_display_flush_ready((lv_display_t *)ctx);
    return false;
}

static void flush_cb(lv_display_t *d, const lv_area_t *area, uint8_t *px)
{
    esp_lcd_panel_io_handle_t io = lv_display_get_user_data(d);
    uint16_t x1 = area->x1, x2 = area->x2, y1 = area->y1, y2 = area->y2;
    esp_lcd_panel_io_tx_param(io, ST7735_CASET,
        (uint8_t[]){x1>>8, x1&0xFF, x2>>8, x2&0xFF}, 4);
    esp_lcd_panel_io_tx_param(io, ST7735_RASET,
        (uint8_t[]){y1>>8, y1&0xFF, y2>>8, y2&0xFF}, 4);
    int sz = (x2-x1+1)*(y2-y1+1)*2;
    esp_lcd_panel_io_tx_color(io, ST7735_RAMWR, px, sz);
}

static void tick_cb(void *arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); }

static lv_display_t *display_init(esp_lcd_panel_io_handle_t io)
{
    lv_display_t *d = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_color_format_t cf = LV_COLOR_FORMAT_RGB565_SWAPPED;
    uint32_t stride = lv_draw_buf_width_to_stride(LCD_H_RES, cf);
    size_t sz = stride * LCD_V_RES;

    void *b1 = spi_bus_dma_memory_alloc(LCD_HOST, sz, 0);
    void *b2 = spi_bus_dma_memory_alloc(LCD_HOST, sz, 0);
    void *b3 = spi_bus_dma_memory_alloc(LCD_HOST, sz, 0);
    assert(b1 && b2 && b3);

    lv_display_set_color_format(d, cf);
    lv_display_set_buffers(d, b1, b2, sz, LV_DISPLAY_RENDER_MODE_FULL);
    lv_draw_buf_init(&s_draw_buf3, LCD_H_RES, LCD_V_RES, cf, stride, b3, sz);
    lv_display_set_3rd_draw_buffer(d, &s_draw_buf3);
    lv_display_set_user_data(d, io);
    lv_display_set_flush_cb(d, flush_cb);
    ESP_LOGI(TAG, "LVGL display: 3 buffers, 60fps");
    return d;
}

/* ── Main ──────────────────────────────────────────────────────────────── */
void app_main(void)
{
    /* Return-to-Loader: must be first line */
    return_to_loader_setup();

    ESP_LOGI(TAG, "XiaoMiao OS booting...");

    /* Initialize all hardware */
    backlight_init();
    backlight_set(255);
    buttons_init();

    /* Power management (battery ADC + filtering) */
    power_init();
    s_battery_voltage = power_read_voltage_filtered();
    ESP_LOGI(TAG, "Battery: %.2fV (%u%%)", s_battery_voltage, power_read_percent());

    esp_lcd_panel_io_handle_t io = lcd_init();

    /* Mount SD card (non-fatal if absent) */
    sd_card_init();

    /* Mount SPIFFS */
    spiffs_init();

    /* I2C + buzzer */
    i2c_init();
    buzzer_init();

    /* LVGL */
    lv_init();
    lv_display_t *disp = display_init(io);

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_display(indev, disp);
    lv_indev_set_group(indev, group);
    lv_indev_set_read_cb(indev, keypad_read_cb);

    /* Register flush callback */
    esp_lcd_panel_io_callbacks_t cbs = { .on_color_trans_done = flush_ready };
    esp_lcd_panel_io_register_event_callbacks(io, &cbs, disp);

    /* LVGL tick timer */
    esp_timer_create_args_t ta = { .callback = tick_cb, .name = "lv" };
    esp_timer_handle_t tt;
    esp_timer_create(&ta, &tt);
    esp_timer_start_periodic(tt, LVGL_TICK_PERIOD_MS * 1000);

    /* Initialize MicroPython runtime */
    micropython_init();

    /* Initialize App Manager (global registry) */
    app_manager_init();
    app_manager_scan_sdcard();

    /* Initialize App Store (PoC: scan /sdcard/store/*.app) */
    app_store_init();
    app_store_scan();

    /* Initialize UI Manager */
    ui_manager_init(&s_ui_manager);

    /* Wait for first flush, then enable display */
    s_first_flush = false;
    lv_refr_now(NULL);
    for (int i = 0; i < 100 && !s_first_flush; i++)
        vTaskDelay(pdMS_TO_TICKS(1));
    st7735_tx(s_lcd_io, ST7735_DISPON, NULL, 0);
    st7735_delay(20);

    /* Startup beep */
    buzzer_beep(880, 64, 100);

    /* Start power monitoring task (low priority) */
    xTaskCreate(power_task, "power", 2048, NULL, 1, &s_power_task_handle);

    ESP_LOGI(TAG, "XiaoMiao OS ready.");

    /* LVGL main loop: also drives periodic battery/statusbar updates */
    uint32_t last_batt_update = 0;
    bool low_warned = false;
    while (true) {
        uint32_t delay = lv_timer_handler();

        /* Update battery/statusbar every 2 seconds */
        uint32_t now = lv_tick_get();
        if (lv_tick_elaps(last_batt_update) >= 2000) {
            float v = power_read_voltage_filtered();
            ui_manager_update_battery(&s_ui_manager, v);
            ui_manager_update_time(&s_ui_manager);
            last_batt_update = now;

            /* Low battery warning */
            power_bat_state_t st = power_read_state();
            if (st == POWER_BAT_LOW) {
                if (!low_warned) {
                    buzzer_beep(440, 32, 50);
                    low_warned = true;
                }
            } else {
                low_warned = false;
            }
        }

        usleep(MAX(MIN(delay, 16), 1) * 1000);
    }
}