/*
 * ui_statusbar.c — Status bar implementation
 */
#include "ui_statusbar.h"
#include "ui_theme.h"
#include <time.h>
#include <stdio.h>

void ui_statusbar_create(ui_statusbar_t *sb, lv_obj_t *parent)
{
    sb->root = lv_obj_create(parent);
    lv_obj_set_size(sb->root, UI_SCREEN_W, UI_STATUSBAR_H);
    lv_obj_set_style_bg_color(sb->root, lv_color_hex(THEME_BROWN), 0);
    lv_obj_set_style_bg_opa(sb->root, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(sb->root, 2, 0);
    lv_obj_set_style_border_width(sb->root, 0, 0);
    lv_obj_set_style_radius(sb->root, 0, 0);
    lv_obj_clear_flag(sb->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Time label (left) */
    sb->time_label = lv_label_create(sb->root);
    lv_obj_set_style_text_color(sb->time_label, lv_color_hex(THEME_CREAM), 0);
    lv_obj_set_style_text_font(sb->time_label, &lv_font_montserrat_10, 0);
    lv_label_set_text(sb->time_label, "12:00");
    lv_obj_align(sb->time_label, LV_ALIGN_LEFT_MID, 2, 0);

    /* Battery indicator (right) */
    sb->batt_bar = lv_obj_create(sb->root);
    lv_obj_set_size(sb->batt_bar, 18, 8);
    lv_obj_set_style_bg_color(sb->batt_bar, lv_color_hex(THEME_BLACK), 0);
    lv_obj_set_style_bg_opa(sb->batt_bar, LV_OPA_20, 0);
    lv_obj_set_style_border_color(sb->batt_bar, lv_color_hex(THEME_CREAM), 0);
    lv_obj_set_style_border_width(sb->batt_bar, 1, 0);
    lv_obj_set_style_radius(sb->batt_bar, 1, 0);
    lv_obj_set_style_pad_all(sb->batt_bar, 1, 0);
    lv_obj_clear_flag(sb->batt_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(sb->batt_bar, LV_ALIGN_RIGHT_MID, -2, 0);

    sb->batt_fill = lv_obj_create(sb->batt_bar);
    lv_obj_set_size(sb->batt_fill, 14, 6);
    lv_obj_set_style_bg_color(sb->batt_fill, lv_color_hex(THEME_GREEN), 0);
    lv_obj_set_style_bg_opa(sb->batt_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(sb->batt_fill, 0, 0);
    lv_obj_set_style_border_width(sb->batt_fill, 0, 0);
    lv_obj_clear_flag(sb->batt_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(sb->batt_fill, LV_ALIGN_LEFT_MID, 0, 0);
}

void ui_statusbar_update_time(ui_statusbar_t *sb)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", tm->tm_hour, tm->tm_min);
    lv_label_set_text(sb->time_label, buf);
}

void ui_statusbar_update_battery(ui_statusbar_t *sb, float voltage)
{
    /* Map 3.3V-4.2V to 0-100% */
    int pct = (int)((voltage - 3.3f) / 0.9f * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    int width = (pct * 14) / 100;
    lv_obj_set_width(sb->batt_fill, width);

    /* Color: red if <20%, green otherwise */
    lv_color_t col = (pct < 20) ? lv_color_hex(THEME_RED) : lv_color_hex(THEME_GREEN);
    lv_obj_set_style_bg_color(sb->batt_fill, col, 0);
}