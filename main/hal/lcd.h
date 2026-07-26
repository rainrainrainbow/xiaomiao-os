#pragma once
#include "driver/spi_master.h"
#include "lvgl.h"

/* ST7735 引脚定义 */
#define LCD_SPI_HOST     SPI2_HOST
#define LCD_PIN_SCLK     18
#define LCD_PIN_MOSI     23
#define LCD_PIN_MISO     19
#define LCD_PIN_CS       5
#define LCD_PIN_DC       4
#define LCD_PIN_RST      -1   /* 硬件复位走 GD32，ESP32 不控制 */
#define LCD_PIN_TE       -1   /* 无 TE 引脚 */

/* 屏幕参数（旋转 90°：原生 128x160 → 显示 160x128） */
#define LCD_HOR_RES      160
#define LCD_VER_RES      128
#define LCD_SPI_CLOCK_HZ 60000000  /* 60MHz */

/* 三缓冲 */
#define LCD_BUF_LINES    42  /* 3 buffers × 42 lines ≈ 128 lines */
#define LCD_NUM_BUFFERS  3

/* 颜色格式 */
#define LCD_COLOR_FORMAT LV_COLOR_FORMAT_RGB565

/* 初始化 / 反初始化 */
esp_err_t lcd_init(void);
void       lcd_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px);
void       lcd_set_backlight(bool on);  /* 实际由 backlight.c 控制 GPIO0 */
lv_display_t *lcd_get_display(void);
