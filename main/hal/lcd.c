/*
 * lcd.c - ST7735S 160x128 TFT 驱动 (SPI2 @ 60MHz)
 *
 * 旋转 90°：原生 128x160 → 显示 160x128
 * 三缓冲 RGB565
 */

#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "hal/lcd.h"

static const char *TAG = "lcd";

static spi_device_handle_t s_spi_dev = NULL;
static lv_display_t *s_disp = NULL;

/* 三缓冲 */
static uint8_t *s_buf[LCD_NUM_BUFFERS] = {NULL, NULL, NULL};
static int s_cur_buf = 0;
static SemaphoreHandle_t s_flush_sem;

/* ---------- ST7735 命令 ---------- */
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

static void lcd_write_cmd(uint8_t cmd)
{
    gpio_set_level(LCD_PIN_DC, 0);
    spi_device_write(s_spi_dev, &cmd, 1);
}

static void lcd_write_data(const uint8_t *data, size_t len)
{
    gpio_set_level(LCD_PIN_DC, 1);
    spi_device_write(s_spi_dev, data, len);
}

static void lcd_write_byte(uint8_t val)
{
    lcd_write_data(&val, 1);
}

static void lcd_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* 设置窗口（旋转90°后 x∈[0,159] y∈[0,127]） */
static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* 旋转 90°：交换 x/y，偏移 (0, 0) */
    uint8_t data[4];
    /* CASET: x 范围 */
    data[0] = (y0 >> 8) & 0xFF; data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF; data[3] = y1 & 0xFF;
    lcd_write_cmd(ST7735_CASET);
    lcd_write_data(data, 4);

    /* RASET: y 范围（旋转后 x 方向） */
    data[0] = (x0 >> 8) & 0xFF; data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF; data[3] = x1 & 0xFF;
    lcd_write_cmd(ST7735_RASET);
    lcd_write_data(data, 4);

    lcd_write_cmd(ST7735_RAMWR);
}

/* ---------- 初始化序列 ---------- */
static void lcd_init_sequence(void)
{
    lcd_write_cmd(ST7735_SWRESET);
    lcd_delay_ms(150);

    lcd_write_cmd(ST7735_SLPOUT);
    lcd_delay_ms(150);

    /* MADCTL: 旋转 90° (MY=1, MX=0, MV=1) */
    lcd_write_cmd(ST7735_MADCTL);
    lcd_write_byte(0xA0);  /* MY|MV, RGB */

    /* COLMOD: 16bpp */
    lcd_write_cmd(ST7735_COLMOD);
    lcd_write_byte(0x05);

    /* FRMCTR1: 帧率 */
    lcd_write_cmd(ST7735_FRMCTR1);
    lcd_write_byte(0x00); lcd_write_byte(0x06); lcd_write_byte(0x03);

    /* INVCTR */
    lcd_write_cmd(ST7735_INVCTR);
    lcd_write_byte(0x07);

    /* 电源控制 */
    lcd_write_cmd(ST7735_PWCTR1);
    lcd_write_byte(0x0A); lcd_write_byte(0x02);
    lcd_write_cmd(ST7735_PWCTR2);
    lcd_write_byte(0x02);
    lcd_write_cmd(ST7735_PWCTR3);
    lcd_write_byte(0x02); lcd_write_byte(0x77);

    /* VCOM */
    lcd_write_cmd(ST7735_VMCTR1);
    lcd_write_byte(0x3B); lcd_write_byte(0x34);

    /* 伽马校正 */
    static const uint8_t gamma_pos[] = {0x0F,0x1A,0x0F,0x18,0x2F,0x28,0x20,0x22,0x1F,0x1B,0x23,0x37,0x00,0x07,0x02,0x10};
    static const uint8_t gamma_neg[] = {0x0F,0x1B,0x0F,0x17,0x33,0x2C,0x29,0x2E,0x30,0x30,0x39,0x3F,0x00,0x07,0x03,0x10};
    lcd_write_cmd(ST7735_GMCTRP1); lcd_write_data(gamma_pos, 16);
    lcd_write_cmd(ST7735_GMCTRN1); lcd_write_data(gamma_neg, 16);

    lcd_delay_ms(10);
    lcd_write_cmd(ST7735_DISPON);
    lcd_delay_ms(100);
}

/* ---------- LVGL flush 回调 ---------- */
static void lcd_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    size_t len = w * h * 2; /* RGB565 */

    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    gpio_set_level(LCD_PIN_DC, 1);
    spi_device_write(s_spi_dev, px_map, len);

    lv_display_flush_ready(disp);
}

/* ---------- SPI 初始化 ---------- */
static esp_err_t lcd_spi_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = LCD_PIN_MISO,
        .sclk_io_num = LCD_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_HOR_RES * LCD_BUF_LINES * 2,
    };
    esp_err_t ret = spi_bus_initialize(LCD_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = LCD_SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = LCD_PIN_CS,
        .queue_size = 3,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    return spi_bus_add_device(LCD_SPI_HOST, &dev_cfg, &s_spi_dev);
}

/* ---------- 公开 API ---------- */
esp_err_t lcd_init(void)
{
    ESP_LOGI(TAG, "Initializing ST7735 (160x128, SPI@60MHz)...");

    /* GPIO */
    gpio_config_t io = {
        .pin_bit_mask = BIT64(LCD_PIN_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(LCD_PIN_DC, 1);

    /* SPI */
    ESP_ERROR_CHECK(lcd_spi_init());

    /* 硬件复位（如果有） */
    if (LCD_PIN_RST >= 0) {
        gpio_set_direction(LCD_PIN_RST, GPIO_MODE_OUTPUT);
        gpio_set_level(LCD_PIN_RST, 0);
        lcd_delay_ms(20);
        gpio_set_level(LCD_PIN_RST, 1);
        lcd_delay_ms(20);
    }

    /* 初始化序列 */
    lcd_init_sequence();

    /* 分配三缓冲 (PSRAM) */
    size_t buf_size = LCD_HOR_RES * LCD_BUF_LINES * 2;
    for (int i = 0; i < LCD_NUM_BUFFERS; i++) {
        s_buf[i] = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_buf[i]) {
            ESP_LOGE(TAG, "Failed to alloc buffer %d (%d bytes)", i, (int)buf_size);
            return ESP_ERR_NO_MEM;
        }
        memset(s_buf[i], 0, buf_size);
    }

    s_flush_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(s_flush_sem);

    /* LVGL display 注册 */
    s_disp = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
    lv_display_set_flush_cb(s_disp, lcd_flush_cb);
    lv_display_set_buffers(s_disp, s_buf[0], s_buf[1],
                           LCD_HOR_RES * LCD_BUF_LINES * 2,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "LCD ready. Buffers: %d x %d lines in PSRAM", LCD_NUM_BUFFERS, LCD_BUF_LINES);
    return ESP_OK;
}

lv_display_t *lcd_get_display(void) { return s_disp; }
