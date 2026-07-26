/*
 * fonts.c - 字体注册
 *
 * 使用 LVGL 内置 Montserrat 字体以节省 Flash 空间。
 * 如需中文，后续可添加 lv_font_load() 从 SD 卡加载。
 */
#include "lvgl.h"
#include "ui/fonts.h"

/* 引用 LVGL 内置字体 */
LV_FONT_DECLARE(lv_font_montserrat_8);
LV_FONT_DECLARE(lv_font_montserrat_10);
LV_FONT_DECLARE(lv_font_montserrat_12);
LV_FONT_DECLARE(lv_font_montserrat_14);

const lv_font_t *FONT_6X8   = &lv_font_montserrat_8;
const lv_font_t *FONT_8X8   = &lv_font_montserrat_10;
const lv_font_t *FONT_12X12 = &lv_font_montserrat_12;
