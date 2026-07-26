#pragma once
#include "lvgl.h"

/* ===== 配色方案 (Windows Phone 风格) ===== */
#define THEME_BG          lv_color_hex(0x1B1B1B)  /* 深灰背景 */
#define THEME_BG_LIGHT    lv_color_hex(0x2B2B2B)
#define THEME_FG          lv_color_hex(0xFFFFFF)  /* 白色前景 */
#define THEME_ACCENT      lv_color_hex(0x00A6FF)  /* 蓝色强调 */
#define THEME_ACCENT2     lv_color_hex(0x7FD858)  /* 绿色 */
#define THEME_WARN        lv_color_hex(0xFF6A00)  /* 橙色警告 */
#define THEME_ERROR       lv_color_hex(0xE81123)  /* 红色错误 */
#define THEME_TILE_1      lv_color_hex(0xF25022)  /* 橙红 */
#define THEME_TILE_2      lv_color_hex(0x7FBA00)  /* 绿 */
#define THEME_TILE_3      lv_color_hex(0x00A6FF)  /* 蓝 */
#define THEME_TILE_4      lv_color_hex(0xFFB900)  /* 金 */
#define THEME_TILE_5      lv_color_hex(0x68217A)  /* 紫 */
#define THEME_TILE_6      lv_color_hex(0x00B294)  /* 青 */
#define THEME_TILE_7      lv_color_hex(0xE74856)  /* 红 */
#define THEME_TILE_8      lv_color_hex(0x10893E)  /* 深绿 */

/* ===== 布局常量 (160x128) ===== */
#define SCREEN_W          160
#define SCREEN_H          128
#define STATUSBAR_H       12
#define NAVBAR_H          14
#define DESKTOP_TOP       STATUSBAR_H
#define DESKTOP_H         (SCREEN_H - STATUSBAR_H - NAVBAR_H)  /* 102 */
#define TILE_SIZE         32
#define TILE_GAP          4
#define TILES_PER_ROW     4
#define TILES_PER_COL     2

/* ===== 字体 ===== */
extern const lv_font_t *FONT_6X8;
extern const lv_font_t *FONT_8X8;
extern const lv_font_t *FONT_12X12;

/* ===== API ===== */
void ui_theme_init(void);
void ui_set_default_style(lv_obj_t *obj);
lv_obj_t *ui_create_tile(lv_obj_t *parent, int x, int y, lv_color_t color, const char *label);
lv_color_t ui_tile_color(int index);
const char *ui_key_name(int key_code);
