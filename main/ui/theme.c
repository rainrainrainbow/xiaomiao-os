// ================ theme.c - 小喵配色主题 ================

#include "theme.h"

const lv_color_t C_YELLOW = LV_COLOR_MAKE(0xF6, 0xD3, 0x4A);
const lv_color_t C_BLACK  = LV_COLOR_MAKE(0x1B, 0x17, 0x13);
const lv_color_t C_BROWN  = LV_COLOR_MAKE(0x5C, 0x42, 0x20);
const lv_color_t C_RED    = LV_COLOR_MAKE(0xE6, 0x4B, 0x3C);
const lv_color_t C_CREAM  = LV_COLOR_MAKE(0xFF, 0xF3, 0xB0);
const lv_color_t C_GREEN  = LV_COLOR_MAKE(0x2D, 0xD4, 0x66);
const lv_color_t C_GREY   = LV_COLOR_MAKE(0x88, 0x88, 0x88);

void theme_init(void)
{
    lv_theme_t *th = lv_theme_default_init(NULL, C_BROWN, C_CREAM,
                                            LV_THEME_DEFAULT_DARK,
                                            LV_FONT_DEFAULT);
    (void)th;  // LVGL 9 由宏自动应用
}

static lv_style_t s_icon_def;
static lv_style_t s_icon_sel;
static lv_style_t s_icon_moving;

void style_apply_icon(lv_obj_t *obj, bool selected, bool moving)
{
    if (!obj) return;
    lv_style_reset(obj);
    if (selected) {
        lv_obj_set_style_bg_color(obj, C_BROWN, 0);
    }
    if (moving) {
        lv_obj_set_style_bg_color(obj, C_RED, 0);
        lv_obj_set_style_transform_angle(obj, 100, 0);  // ~6°
    }
    lv_obj_set_style_radius(obj, 4, 0);
    lv_obj_set_style_pad_all(obj, 2, 0);
}

static lv_style_t s_li_def;
static lv_style_t s_li_sel;

void style_apply_list_item(lv_obj_t *obj, bool selected)
{
    if (!obj) return;
    lv_obj_set_style_bg_color(obj, selected ? C_BROWN : C_YELLOW, 0);
    lv_obj_set_style_text_color(obj, selected ? C_CREAM : C_BLACK, 0);
    lv_obj_set_style_radius(obj, 2, 0);
    lv_obj_set_style_pad_hor(obj, 3, 0);
}

void style_apply_titlebar(lv_obj_t *obj)
{
    if (!obj) return;
    lv_obj_set_style_bg_color(obj, C_BROWN, 0);
    lv_obj_set_style_text_color(obj, C_CREAM, 0);
    lv_obj_set_style_pad_hor(obj, 4, 0);
    lv_obj_set_size(obj, LV_HOR_RES, 12);
}

void style_apply_statusbar(lv_obj_t *obj)
{
    if (!obj) return;
    lv_obj_set_style_bg_color(obj, C_BROWN, 0);
    lv_obj_set_style_text_color(obj, C_CREAM, 0);
    lv_obj_set_size(obj, LV_HOR_RES, 12);
    lv_obj_set_style_pad_hor(obj, 3, 0);
}