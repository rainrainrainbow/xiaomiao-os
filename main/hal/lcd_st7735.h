// ================ lcd_st7735.h - ST7735 160x128 SPI 驱动 + 背光控制 ================
// 小喵掌机: SPI2 @ 15MHz, CS=5, DC=4, RST=-1
// 背光: GPIO0, **低电平点亮**, 通过 LEDC 软件控亮度（PWM 占空比 0~100）
//
// 修正（v0.2）：背光由硬接 VCC 改为 GPIO0 控, 可实现设置里的"屏幕亮度"真实生效
// 修正（v0.3）：SPI 频率从 60MHz 降至 15MHz（ST7735 最大规格），修复字节序（小端→大端）

#ifndef __LCD_ST7735_H__
#define __LCD_ST7735_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_PIN_SCLK  18
#define LCD_PIN_MOSI  23
#define LCD_PIN_MISO  19
#define LCD_PIN_CS     5
#define LCD_PIN_DC     4
#define LCD_PIN_RST   -1

// 背光 GPIO（低电平点亮）
#define LCD_PIN_BL    0

#define LCD_SPI_FREQ_HZ  (15 * 1000 * 1000)   // ST7735 最大 15MHz

#define LCD_HRES  160
#define LCD_VRES  128
#define LCD_PIXELS  (LCD_HRES * LCD_VRES)

// ================ 颜色（RGB565） ================
#define COLOR_YELLOW  0xF6D4
#define COLOR_BLACK   0x0000
#define COLOR_BROWN   0x5280
#define COLOR_RED     0xE253
#define COLOR_CREAM   0xFFB0
#define COLOR_GREEN   0x2DD4
#define COLOR_GREY    0x7BEF

typedef struct {
    uint8_t *buf_a;
    uint8_t *buf_b;
    uint8_t *buf_c;
    uint8_t front;
} lcd_buffers_t;

esp_err_t lcd_init(void);
void       lcd_flush(uint16_t *pixel_buf, uint32_t pixel_count);
void       lcd_flush_area(uint16_t *pixel_buf, int x0, int y0, int x1, int y1);

// ================ 背光控制 (LEDC 占空比) ================
//
//  注意: GPIO0 低电平点亮, 所以 duty=0 → 100% 亮度（占空比 = 高电平时间）
//        duty=100 → 0% 亮度
//        用户看到的 0-100% 亮度是 *反向* 映射, 这里参数统一用 "亮度" 0-100
//
esp_err_t lcd_backlight_init(void);
void       lcd_set_brightness(uint8_t pct);   // 0=灭, 100=最亮
uint8_t    lcd_get_brightness(void);

void       lcd_sleep(void);
void       lcd_wake(void);

lcd_buffers_t *lcd_buffers_get(void);
uint8_t *lcd_front_buffer_take(void);

#ifdef __cplusplus
}
#endif

#endif // __LCD_ST7735_H__