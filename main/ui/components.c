/*
 * components.c - 通用 UI 组件
 */
#include "lvgl.h"
#include "ui/components.h"
#include "ui/theme.h"

/* ---------- 磁贴按钮 ---------- */
lv_obj_t *ui_button_create(lv_obj_t *parent, const char *text, lv_color_t bg_color,
                           int x, int y, int w, int h)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, bg_color, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 2, 0);

    if (text) {
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, text);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    }
    return btn;
}

void ui_button_set_callback(lv_obj_t *btn, lv_event_cb_t cb)
{
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
}

/* ---------- 列表项 ---------- */
lv_obj_t *ui_list_item_create(lv_obj_t *parent, const char *text, const char *subtext,
                              lv_color_t accent)
{
    int parent_w = lv_obj_get_width(parent);
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_set_size(item, parent_w - 4, 20);
    lv_obj_set_style_bg_color(item, THEME_BG_LIGHT, 0);
    lv_obj_set_style_radius(item, 0, 0);
    lv_obj_set_style_border_width(item, 0, 0);
    lv_obj_set_style_pad_all(item, 2, 0);

    /* 左侧色条 */
    lv_obj_t *bar = lv_obj_create(item);
    lv_obj_set_size(bar, 3, 16);
    lv_obj_set_pos(bar, 1, 2);
    lv_obj_set_style_bg_color(bar, accent, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);

    /* 主文本 */
    lv_obj_t *lbl = lv_label_create(item);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, THEME_FG, 0);
    lv_obj_set_style_text_font(lbl, FONT_6X8, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 6, -3);

    /* 副文本 */
    if (subtext) {
        lv_obj_t *sub = lv_label_create(item);
        lv_label_set_text(sub, subtext);
        lv_obj_set_style_text_color(sub, lv_color_hex(0x888888), 0);
        lv_obj_set_style_text_font(sub, FONT_6X8, 0);
        lv_obj_align(sub, LV_ALIGN_LEFT_MID, 6, 7);
    }
    return item;
}

/* ---------- 进度条 ---------- */
lv_obj_t *ui_progress_create(lv_obj_t *parent, int w, int h)
{
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_set_size(bar, w, h);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_bg_color(bar, THEME_ACCENT, LV_PART_INDICATOR);
    return bar;
}

void ui_progress_set(lv_obj_t *bar, int value)
{
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    lv_bar_set_value(bar, value, LV_ANIM_ON);
}

/* ---------- 对话框 ---------- */
static void ui_alert_close_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_user_data(e);
    lv_obj_delete(mbox);
}

void ui_alert(const char *title, const char *msg)
{
    lv_obj_t *bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 140, 60);
    lv_obj_center(bg);
    lv_obj_set_style_bg_color(bg, THEME_BG_LIGHT, 0);
    lv_obj_set_style_border_color(bg, THEME_ACCENT, 0);
    lv_obj_set_style_border_width(bg, 1, 0);

    lv_obj_t *t = lv_label_create(bg);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, THEME_ACCENT, 0);
    lv_obj_set_style_text_font(t, FONT_8X8, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *m = lv_label_create(bg);
    lv_label_set_text(m, msg);
    lv_obj_set_style_text_color(m, THEME_FG, 0);
    lv_obj_set_style_text_font(m, FONT_6X8, 0);
    lv_obj_align(m, LV_ALIGN_CENTER, 0, 4);

    lv_obj_t *btn = lv_btn_create(bg);
    lv_obj_set_size(btn, 40, 14);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(btn, THEME_ACCENT, 0);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "OK");
    lv_obj_add_event_cb(btn, ui_alert_close_cb, LV_EVENT_CLICKED, bg);
}

void ui_confirm(const char *title, const char *msg, lv_event_cb_t on_yes)
{
    lv_obj_t *bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 140, 60);
    lv_obj_center(bg);
    lv_obj_set_style_bg_color(bg, THEME_BG_LIGHT, 0);
    lv_obj_set_style_border_color(bg, THEME_WARN, 0);
    lv_obj_set_style_border_width(bg, 1, 0);

    lv_obj_t *t = lv_label_create(bg);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, THEME_WARN, 0);
    lv_obj_set_style_text_font(t, FONT_8X8, 0);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *m = lv_label_create(bg);
    lv_label_set_text(m, msg);
    lv_obj_set_style_text_color(m, THEME_FG, 0);
    lv_obj_set_style_text_font(m, FONT_6X8, 0);
    lv_obj_align(m, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn = lv_btn_create(bg);
    lv_obj_set_size(btn, 40, 14);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(btn, THEME_WARN, 0);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "Yes");
    lv_obj_add_event_cb(btn, on_yes, LV_EVENT_CLICKED, bg);
}

/* ---------- 图标（符号文字） ---------- */
lv_obj_t *ui_icon_create(lv_obj_t *parent, const char *symbol, lv_color_t color, int size)
{
    lv_obj_t *icon = lv_label_create(parent);
    lv_label_set_text(icon, symbol);
    lv_obj_set_style_text_color(icon, color, 0);
    lv_obj_set_style_text_font(icon, size <= 8 ? FONT_8X8 : FONT_12X12, 0);
    return icon;
}

/* ---------- 标签快捷 ---------- */
lv_obj_t *ui_label_create(lv_obj_t *parent, const char *text, lv_color_t color,
                          const lv_font_t *font, lv_align_t align, int x, int y)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_style_text_font(lbl, font ? font : FONT_6X8, 0);
    lv_obj_align(lbl, align, x, y);
    return lbl;
}
