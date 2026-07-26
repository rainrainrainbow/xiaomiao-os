/*
 * theme.c - UI 主题初始化
 *
 * 配色: Windows Phone 风格磁贴
 * 字体: 内置 LVGL 默认字体 (编译体积优化)
 */
#include "lvgl.h"
#include "ui/theme.h"

/* 使用 LVGL 内置字体 */
const lv_font_t *FONT_6X8   = &lv_font_montserrat_8;
const lv_font_t *FONT_8X8   = &lv_font_montserrat_10;
const lv_font_t *FONT_12X12 = &lv_font_montserrat_12;

void ui_theme_init(void)
{
    /* 设置默认样式 */
    static lv_style_t style_default;
    lv_style_init(&style_default);
    lv_style_set_bg_color(&style_default, THEME_BG);
    lv_style_set_text_color(&style_default, THEME_FG);
    lv_style_set_text_font(&style_default, FONT_6X8);
    lv_style_set_border_width(&style_default, 0);
    lv_style_set_pad_all(&style_default, 0);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_add_style(scr, &style_default, 0);
    lv_obj_set_style_bg_color(scr, THEME_BG, 0);

    /* 滚动条隐藏（小屏不需要） */
    lv_obj_set_style_opa(lv_screen_active(), LV_OPA_COVER, 0);
}

void ui_set_default_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, THEME_BG, 0);
    lv_obj_set_style_text_color(obj, THEME_FG, 0);
    lv_obj_set_style_text_font(obj, FONT_6X8, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

lv_color_t ui_tile_color(int index)
{
    static const lv_color_t colors[] = {
        THEME_TILE_1, THEME_TILE_2, THEME_TILE_3, THEME_TILE_4,
        THEME_TILE_5, THEME_TILE_6, THEME_TILE_7, THEME_TILE_8,
    };
    return colors[index % 8];
}

const char *ui_key_name(int key_code)
{
    switch (key_code) {
        case 1: return "UP"; case 2: return "DOWN";
        case 3: return "LEFT"; case 4: return "RIGHT";
        case 5: return "A"; case 6: return "B";
        default: return "?";
    }
}

lv_obj_t *ui_create_tile(lv_obj_t *parent, int x, int y, lv_color_t color, const char *label)
{
    lv_obj_t *tile = lv_obj_create(parent);
    lv_obj_set_size(tile, TILE_SIZE, TILE_SIZE);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_style_bg_color(tile, color, 0);
    lv_obj_set_style_radius(tile, 0, 0);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_style_pad_all(tile, 2, 0);

    if (label) {
        lv_obj_t *lbl = lv_label_create(tile);
        lv_label_set_text(lbl, label);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    return tile;
}
