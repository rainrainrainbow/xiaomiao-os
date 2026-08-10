// ================ lcd_st7735.c - ST7735 + GPIO0 背光 LEDC ================

#include "lcd_st7735.h"
#include "esp_log.h"
#include "esp_check.h"        // 提供 ESP_RETURN_ON_ERROR
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lcd_st7735";

static spi_device_handle_t s_spi_dev = NULL;
static lcd_buffers_t s_bufs = { 0 };
static uint8_t s_backlight_pct = 100;   // 默认最亮

// ================ 低层 SPI ================
static esp_err_t lcd_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
        .user = (void *)0,  // DC=0
    };
    return spi_device_polling_transmit(s_spi_dev, &t);
}
static esp_err_t lcd_data(const uint8_t *data, int len)
{
    if (len == 0) return ESP_OK;
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
        .user = (void *)1,  // DC=1
    };
    return spi_device_polling_transmit(s_spi_dev, &t);
}
static void IRAM_ATTR lcd_pre_cb(spi_transaction_t *t)
{
    gpio_set_level(LCD_PIN_DC, (int)t->user & 1);
}

// ================ 寄存器初始化序列 ================
typedef struct { uint8_t cmd; uint8_t len; const uint8_t *data; } lcd_cmd_t;

static const uint8_t kGmaPos[] = {
    0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
    0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10
};
static const uint8_t kGmaNeg[] = {
    0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
    0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10
};
static const uint8_t kFrmctrl3[] = {0x01, 0x2C, 0x01, 0x2C};
static const uint8_t kPwctr1[] = {0xA2, 0x02, 0x84};
static const uint8_t kXset[] = {0x00, 0x00, 0x00, 0x7F};
static const uint8_t kYset[] = {0x00, 0x00, 0x00, 0x9F};

static const lcd_cmd_t kInitSeq[] = {
    {0x01, 0, NULL},
    {0x11, 0, NULL},
    {0xB1, 2, (const uint8_t[]){0x01, 0x2C}},
    {0xB2, 2, (const uint8_t[]){0x01, 0x2C}},
    {0xB3, 4, kFrmctrl3},
    {0xB4, 1, (const uint8_t[]){0x07}},
    {0xC0, 3, kPwctr1},
    {0xC1, 1, (const uint8_t[]){0xC5}},
    {0xC2, 2, (const uint8_t[]){0x0A, 0x00}},
    {0xC3, 2, (const uint8_t[]){0x8A, 0x2A}},
    {0xC4, 2, (const uint8_t[]){0x8A, 0xEE}},
    {0xC5, 1, (const uint8_t[]){0x0E}},
    {0x3A, 1, (const uint8_t[]){0x05}},        // RGB565
    {0x36, 1, (const uint8_t[]){0xC0}},        // 横屏
    {0x2A, 4, kXset},
    {0x2B, 4, kYset},
    {0xE0, 16, kGmaPos},
    {0xE1, 16, kGmaNeg},
    {0x13, 0, NULL},
    {0x29, 0, NULL},
};

static esp_err_t lcd_apply_init_seq(void)
{
    for (size_t i = 0; i < sizeof(kInitSeq)/sizeof(kInitSeq[0]); i++) {
        const lcd_cmd_t *c = &kInitSeq[i];
        ESP_RETURN_ON_ERROR(lcd_cmd(c->cmd), TAG, "cmd 0x%02X fail", c->cmd);
        if (c->len > 0 && c->data) {
            ESP_RETURN_ON_ERROR(lcd_data(c->data, c->len), TAG, "data cmd 0x%02X fail", c->cmd);
        }
    }
    return ESP_OK;
}

// ================ 背光（LEDC Timer1 Channel0） ================
//   GPIO0 低电平点亮:
//     亮度 = 100%   → duty 0   (常低)
//     亮度 = 0%     → duty 256 (常高) - 灭
//
//   所以实际亮度映射为: active_duty = 256 * (100 - pct) / 100
//
esp_err_t lcd_backlight_init(void)
{
    ledc_timer_config_t tcfg = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,   // 避开 Timer0 (蜂鸣器)
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&tcfg), TAG, "bl timer");

    ledc_channel_config_t ccfg = {
        .channel    = LEDC_CHANNEL_1,   // 避开 Ch0 (蜂鸣器)
        .duty       = 0,                 // 100% 亮度
        .gpio_num   = LCD_PIN_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel  = LEDC_TIMER_1,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ccfg), TAG, "bl channel");

    s_backlight_pct = 100;
    ESP_LOGI(TAG, "backlight init OK (GPIO%d, low-active LEDC T1/C1)", LCD_PIN_BL);
    return ESP_OK;
}

void lcd_set_brightness(uint8_t pct)
{
    if (pct > 100) pct = 100;
    s_backlight_pct = pct;
    // 反向: 100-pct 高电平比例
    uint32_t duty = (256UL * (100 - pct)) / 100UL;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

uint8_t lcd_get_brightness(void) { return s_backlight_pct; }

// ================ 公共 API ================
esp_err_t lcd_init(void)
{
    // 1. SPI 总线
    spi_bus_config_t buscfg = {
        .miso_io_num = LCD_PIN_MISO,
        .mosi_io_num = LCD_PIN_MOSI,
        .sclk_io_num = LCD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_PIXELS * 2 + 8,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO),
                        TAG, "spi bus");

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = LCD_SPI_FREQ_HZ,
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .flags = SPI_DEVICE_NO_DUMMY,
        .queue_size = 7,
        .pre_cb = lcd_pre_cb,
    };
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_dev), TAG, "spi dev");

    gpio_config_t dc_conf = {
        .pin_bit_mask = (1ULL << LCD_PIN_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&dc_conf);

    // 2. 分配三缓冲
    s_bufs.buf_a = heap_caps_malloc(LCD_PIXELS * 2, MALLOC_CAP_SPIRAM);
    s_bufs.buf_b = heap_caps_malloc(LCD_PIXELS * 2, MALLOC_CAP_SPIRAM);
    s_bufs.buf_c = heap_caps_malloc(LCD_PIXELS * 2, MALLOC_CAP_SPIRAM);
    if (!s_bufs.buf_a || !s_bufs.buf_b || !s_bufs.buf_c) {
        ESP_LOGE(TAG, "PSRAM alloc fail");
        return ESP_ERR_NO_MEM;
    }
    memset(s_bufs.buf_a, 0, LCD_PIXELS * 2);
    memset(s_bufs.buf_b, 0, LCD_PIXELS * 2);
    memset(s_bufs.buf_c, 0, LCD_PIXELS * 2);

    // 3. ST7735 寄存器序列
    ESP_RETURN_ON_ERROR(lcd_apply_init_seq(), TAG, "init seq");

    // 4. 背光（默认满亮度）
    ESP_RETURN_ON_ERROR(lcd_backlight_init(), TAG, "bl init");
    lcd_set_brightness(100);

    ESP_LOGI(TAG, "LCD init OK (160x128 RGB565, 3-buffered, backlit GPIO%d)", LCD_PIN_BL);
    return ESP_OK;
}

void lcd_flush(uint16_t *pixel_buf, uint32_t pixel_count)
{
    if (!pixel_buf || pixel_count != LCD_PIXELS) return;
    lcd_cmd(0x2A);
    lcd_data((const uint8_t[]){0x00, 0x00, 0x00, 0x7F}, 4);
    lcd_cmd(0x2B);
    lcd_data((const uint8_t[]){0x00, 0x00, 0x00, 0x9F}, 4);
    lcd_cmd(0x2C);

    spi_transaction_t t = {
        .length = LCD_PIXELS * 2 * 8,
        .tx_buffer = pixel_buf,
        .user = (void *)1,
    };
    spi_device_polling_transmit(s_spi_dev, &t);
}

void lcd_sleep(void)
{
    lcd_cmd(0x28);
    lcd_cmd(0x10);
    lcd_set_brightness(0);
}
void lcd_wake(void)
{
    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    lcd_cmd(0x29);
    lcd_set_brightness(s_backlight_pct);
}

lcd_buffers_t *lcd_buffers_get(void) { return &s_bufs; }

uint8_t *lcd_front_buffer_take(void)
{
    uint8_t *buf = NULL;
    switch (s_bufs.front) {
        case 0: buf = s_bufs.buf_a; break;
        case 1: buf = s_bufs.buf_b; break;
        case 2: buf = s_bufs.buf_c; break;
    }
    s_bufs.front = (s_bufs.front + 1) % 3;
    return buf;
}