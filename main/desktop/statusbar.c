/*
 * statusbar.c - 顶部状态栏
 *
 * 布局 (160x12):
 *   [时间 左对齐]          [电量图标 右对齐]
 */
#include "lvgl.h"
#include "desktop/statusbar.h"
#include "ui/theme.h"
#include "hal/battery.h"

static lv_obj_t *s_statusbar = NULL;
static lv_obj_t *s_time_lbl = NULL;
static lv_obj_t *s_battery_lbl = NULL;

/* 简易时间（基于 tick 计数，无 RTC） */
static uint32_t s_seconds = 0;
static uint32_t s_last_tick = 0;

void statusbar_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    s_statusbar = lv_obj_create(scr);
    lv_obj_set_size(s_statusbar, SCREEN_W, STATUSBAR_H);
    lv_obj_set_pos(s_statusbar, 0, 0);
    lv_obj_set_style_bg_color(s_statusbar, THEME_BG_LIGHT, 0);
    lv_obj_set_style_radius(s_statusbar, 0, 0);
    lv_obj_set_style_border_width(s_statusbar, 0, 0);
    lv_obj_set_style_pad_all(s_statusbar, 1, 0);

    /* 时间 */
    s_time_lbl = lv_label_create(s_statusbar);
    lv_label_set_text(s_time_lbl, "00:00");
    lv_obj_set_style_text_color(s_time_lbl, THEME_FG, 0);
    lv_obj_set_style_text_font(s_time_lbl, FONT_6X8, 0);
    lv_obj_align(s_time_lbl, LV_ALIGN_LEFT_MID, 2, 0);

    /* 电量 */
    s_battery_lbl = lv_label_create(s_statusbar);
    lv_label_set_text(s_battery_lbl, "100%");
    lv_obj_set_style_text_color(s_battery_lbl, THEME_ACCENT2, 0);
    lv_obj_set_style_text_font(s_battery_lbl, FONT_6X8, 0);
    lv_obj_align(s_battery_lbl, LV_ALIGN_RIGHT_MID, -2, 0);

    s_last_tick = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void statusbar_update(void)
{
    if (!s_statusbar) return;

    /* 更新时间 */
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_seconds += (now - s_last_tick) / 1000;
    s_last_tick = now;

    uint32_t hh = (s_seconds / 3600) % 24;
    uint32_t mm = (s_seconds / 60) % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu", hh, mm);
    lv_label_set_text(s_time_lbl, buf);

    /* 更新电量 */
    uint8_t pct = battery_percentage();
    const char *status = battery_status_text();
    snprintf(buf, sizeof(buf), "%d%% %s", pct, status);
    lv_label_set_text(s_battery_lbl, buf);

    lv_color_t c = pct > 50 ? THEME_ACCENT2 : (pct > 20 ? THEME_WARN : THEME_ERROR);
    lv_obj_set_style_text_color(s_battery_lbl, c, 0);
}

void statusbar_set_time(const char *time_str)
{
    if (s_time_lbl) lv_label_set_text(s_time_lbl, time_str);
}

void statusbar_set_battery(int percent)
{
    if (s_battery_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", percent);
        lv_label_set_text(s_battery_lbl, buf);
    }
}
