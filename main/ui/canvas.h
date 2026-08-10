// ================ canvas.h - MicroPython 画布（独占 LCD 屏） ================
//
//  在 SCREEN_APP 屏上创建一个 160×128 的画布 (lv_canvas)
//  Python App 调用 hal.display_fill / text / set_pixel 修改画布
//  退出 App 时释放画布.

#ifndef __CANVAS_H__
#define __CANVAS_H__

#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 在 SCREEN_APP 屏上创建/销毁画布
 */
esp_err_t canvas_create(void);
void       canvas_destroy(void);

/**
 * @brief 填充画布
 */
void canvas_fill(uint16_t color);
void canvas_set_pixel(int x, int y, uint16_t color);
void canvas_text(int x, int y, const char *str);

/**
 * @brief 推送画布到屏（lvgl flush 触发）
 */
void canvas_flush(void);

/** 给 Python 调用: 取最近按键 char ('U'/'D'/'L'/'R'/'A'/'B') */
char button_peek_char(void);

/** 阻塞直到按键（供 buttons_wait） */
void button_wait_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // __CANVAS_H__