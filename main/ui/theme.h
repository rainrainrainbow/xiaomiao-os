// ================ theme.h - 小喵原厂配色 ================

#ifndef __THEME_H__
#define __THEME_H__

#include "lvgl.h"

// 原厂黄、棕、黑、红、奶油、绿（16-bit RGB565）
#define COLOR_YELLOW  0xF6D4
#define COLOR_BLACK   0x0000
#define COLOR_BROWN   0x5280
#define COLOR_RED     0xE253
#define COLOR_CREAM   0xFFB0
#define COLOR_GREEN   0x2DD4
#define COLOR_GREY    0x7BEF

// 应用 LVGL 颜色结构体（定义在 theme.c，避免重复定义 -fno-common）
extern const lv_color_t C_YELLOW;
extern const lv_color_t C_BLACK;
extern const lv_color_t C_BROWN;
extern const lv_color_t C_RED;
extern const lv_color_t C_CREAM;
extern const lv_color_t C_GREEN;
extern const lv_color_t C_GREY;

/**
 * @brief 初始化 LVGL 默认主题（小喵风格）
 *        - 深棕背景
 *        - 黄色主体
 *        - 圆角 4px
 */
void theme_init(void);

/**
 * @brief 应用某一风格的样式到对象
 */
void style_apply_icon(lv_obj_t *obj, bool selected, bool moving);
void style_apply_list_item(lv_obj_t *obj, bool selected);
void style_apply_titlebar(lv_obj_t *obj);
void style_apply_statusbar(lv_obj_t *obj);

#endif // __THEME_H__