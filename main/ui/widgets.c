/*
 * widgets.c - 高级 UI 控件
 */
#include "lvgl.h"
#include "ui/widgets.h"
#include "ui/theme.h"

/* ---------- 导航栏 ---------- */
lv_obj_t *widgets_navbar_create(lv_obj_t *parent)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, SCREEN_W, NAVBAR_H);
    lv_obj_set_pos(bar, 0, SCREEN_H - NAVBAR_H);
    lv_obj_set_style_bg_color(bar, THEME_BG_LIGHT, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 1, 0);
    return bar;
}

void widgets_navbar_set(lv_obj_t *bar, const char *left, const char *center, const char *right)
{
    if (!bar) return;
    lv_obj_clean(bar);

    if (left) {
        lv_obj_t *l = lv_label_create(bar);
        lv_label_set_text(l, left);
        lv_obj_set_style_text_color(l, THEME_ACCENT, 0);
        lv_obj_set_style_text_font(l, FONT_6X8, 0);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 4, 0);
    }
    if (center) {
        lv_obj_t *c = lv_label_create(bar);
        lv_label_set_text(c, center);
        lv_obj_set_style_text_color(c, THEME_FG, 0);
        lv_obj_set_style_text_font(c, FONT_6X8, 0);
        lv_obj_align(c, LV_ALIGN_CENTER, 0, 0);
    }
    if (right) {
        lv_obj_t *r = lv_label_create(bar);
        lv_label_set_text(r, right);
        lv_obj_set_style_text_color(r, THEME_ACCENT, 0);
        lv_obj_set_style_text_font(r, FONT_6X8, 0);
        lv_obj_align(r, LV_ALIGN_RIGHT_MID, -4, 0);
    }
}

/* ---------- 弹出菜单 ---------- */
lv_obj_t *widgets_popup_create(const char *title)
{
    lv_obj_t *bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 140, 80);
    lv_obj_center(bg);
    lv_obj_set_style_bg_color(bg, THEME_BG_LIGHT, 0);
    lv_obj_set_style_border_color(bg, THEME_ACCENT, 0);
    lv_obj_set_style_border_width(bg, 1, 0);

    if (title) {
        lv_obj_t *t = lv_label_create(bg);
        lv_label_set_text(t, title);
        lv_obj_set_style_text_color(t, THEME_ACCENT, 0);
        lv_obj_set_style_text_font(t, FONT_8X8, 0);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);
    }
    return bg;
}

void widgets_popup_add_item(lv_obj_t *popup, const char *text, lv_event_cb_t cb)
{
    if (!popup) return;
    /* 简化: 直接创建标签 */
    lv_obj_t *item = lv_label_create(popup);
    lv_label_set_text(item, text);
    lv_obj_set_style_text_color(item, THEME_FG, 0);
    lv_obj_set_style_text_font(item, FONT_6X8, 0);
}

void widgets_popup_close(lv_obj_t *popup)
{
    if (popup) lv_obj_delete(popup);
}

/* ---------- 滑动条 ---------- */
lv_obj_t *widgets_slider_create(lv_obj_t *parent, int x, int y, int w, int min, int max, int val)
{
    lv_obj_t *s = lv_bar_create(parent);
    lv_obj_set_size(s, w, 8);
    lv_obj_set_pos(s, x, y);
    lv_bar_set_range(s, min, max);
    lv_bar_set_value(s, val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(s, 0, 0);
    lv_obj_set_style_border_width(s, 0, 0);
    lv_obj_set_style_bg_color(s, THEME_ACCENT, LV_PART_INDICATOR);
    return s;
}

void widgets_slider_set(lv_obj_t *slider, int val) { lv_bar_set_value(slider, val, LV_ANIM_ON); }
int  widgets_slider_get(lv_obj_t *slider) { return lv_bar_get_value(slider); }

/* ---------- 开关 ---------- */
lv_obj_t *widgets_toggle_create(lv_obj_t *parent, int x, int y, bool on)
{
    lv_obj_t *t = lv_obj_create(parent);
    lv_obj_set_size(t, 24, 12);
    lv_obj_set_pos(t, x, y);
    lv_obj_set_style_bg_color(t, on ? THEME_ACCENT2 : lv_color_hex(0x444444), 0);
    lv_obj_set_style_radius(t, 6, 0);
    lv_obj_set_style_border_width(t, 0, 0);
    return t;
}

bool widgets_toggle_get(lv_obj_t *toggle)
{
    lv_color_t c = lv_obj_get_style_bg_color(toggle, 0);
    return (c.full == THEME_ACCENT2.full);
}

/* ---------- 图标按钮 ---------- */
lv_obj_t *widgets_icon_btn_create(lv_obj_t *parent, const char *symbol, lv_color_t bg, int x, int y, int size)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_radius(btn, size/4, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    if (symbol) {
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, symbol);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, FONT_8X8, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    return btn;
}
