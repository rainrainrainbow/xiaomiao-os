/*
 * settings.c - 设置页面
 *
 * 菜单项:
 *   亮度调节 (条)
 *   电池信息
 *   存储信息
 *   Wi-Fi (TODO)
 *   恢复出厂
 */
#include "lvgl.h"
#include "desktop/settings.h"
#include "ui/theme.h"
#include "ui/components.h"
#include "ui/page_manager.h"
#include "hal/backlight.h"
#include "hal/battery.h"
#include "hal/sdcard.h"
#include "desktop/desktop.h"

static const char *TAG = "settings";

static lv_obj_t *s_settings_root = NULL;
static lv_obj_t *s_brightness_bar = NULL;
static lv_obj_t *s_battery_label = NULL;
static lv_obj_t *s_storage_label = NULL;
static int       s_cursor = 0;
static int       s_brightness = 255;

#define SETTINGS_ITEMS 5
static const char *ITEMS[SETTINGS_ITEMS] = {
    "亮度调节", "电池信息", "存储信息", "Wi-Fi", "恢复出厂"
};

void settings_open(void)
{
    lv_obj_t *scr = lv_screen_active();

    s_settings_root = lv_obj_create(scr);
    lv_obj_set_size(s_settings_root, SCREEN_W, SCREEN_H - STATUSBAR_H);
    lv_obj_set_pos(s_settings_root, 0, STATUSBAR_H);
    lv_obj_set_style_bg_color(s_settings_root, THEME_BG, 0);
    lv_obj_set_style_border_width(s_settings_root, 0, 0);
    lv_obj_set_style_pad_all(s_settings_root, 4, 0);

    /* 标题 */
    lv_obj_t *title = lv_label_create(s_settings_root);
    lv_label_set_text(title, "设置");
    lv_obj_set_style_text_color(title, THEME_ACCENT, 0);
    lv_obj_set_style_text_font(title, FONT_8X8, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 2);

    /* 亮度 */
    lv_obj_t *bl = lv_label_create(s_settings_root);
    lv_label_set_text(bl, "亮度:");
    lv_obj_set_style_text_color(bl, THEME_FG, 0);
    lv_obj_align(bl, LV_ALIGN_TOP_LEFT, 4, 18);

    s_brightness_bar = ui_progress_create(s_settings_root, 100, 8);
    lv_obj_align(s_brightness_bar, LV_ALIGN_TOP_LEFT, 36, 19);
    ui_progress_set(s_brightness_bar, 100);

    /* 电池 */
    s_battery_label = lv_label_create(s_settings_root);
    char buf[64];
    float v = battery_voltage();
    snprintf(buf, sizeof(buf), "电池: %.2fV (%d%%) %s",
             v, battery_percentage(), battery_status_text());
    lv_label_set_text(s_battery_label, buf);
    lv_obj_set_style_text_color(s_battery_label, THEME_FG, 0);
    lv_obj_set_style_text_font(s_battery_label, FONT_6X8, 0);
    lv_obj_align(s_battery_label, LV_ALIGN_TOP_LEFT, 4, 32);

    /* 存储 */
    s_storage_label = lv_label_create(s_settings_root);
    bool mounted = sdcard_is_mounted();
    snprintf(buf, sizeof(buf), "存储: %s", mounted ? "SD卡已挂载" : "SD卡未插入");
    lv_label_set_text(s_storage_label, buf);
    lv_obj_set_style_text_color(s_storage_label, THEME_FG, 0);
    lv_obj_set_style_text_font(s_storage_label, FONT_6X8, 0);
    lv_obj_align(s_storage_label, LV_ALIGN_TOP_LEFT, 4, 46);

    /* WiFi */
    lv_obj_t *wifi = lv_label_create(s_settings_root);
    lv_label_set_text(wifi, "Wi-Fi: 未配置");
    lv_obj_set_style_text_color(wifi, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(wifi, FONT_6X8, 0);
    lv_obj_align(wifi, LV_ALIGN_TOP_LEFT, 4, 60);

    /* 恢复出厂 */
    lv_obj_t *reset = lv_label_create(s_settings_root);
    lv_label_set_text(reset, "恢复出厂: 按住B 5秒");
    lv_obj_set_style_text_color(reset, THEME_WARN, 0);
    lv_obj_set_style_text_font(reset, FONT_6X8, 0);
    lv_obj_align(reset, LV_ALIGN_TOP_LEFT, 4, 74);

    /* 底部提示 */
    lv_obj_t *hint = lv_label_create(s_settings_root);
    lv_label_set_text(hint, "A:返回桌面");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, FONT_6X8, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -2);

    page_register(PAGE_SETTINGS, s_settings_root);
    page_show(PAGE_SETTINGS);

    /* 隐藏桌面 */
    lv_obj_add_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "Settings opened");
}

void settings_handle_key(int key)
{
    switch (key) {
        case 1: /* UP */   if (s_cursor > 0) s_cursor--; break;
        case 2: /* DOWN */ if (s_cursor < SETTINGS_ITEMS - 1) s_cursor++; break;
        case 3: /* LEFT - 亮度降低 */
            s_brightness = (s_brightness > 25) ? s_brightness - 25 : 0;
            backlight_set(s_brightness);
            ui_progress_set(s_brightness_bar, s_brightness * 100 / 255);
            break;
        case 4: /* RIGHT - 亮度升高 */
            s_brightness = (s_brightness < 230) ? s_brightness + 25 : 255;
            backlight_set(s_brightness);
            ui_progress_set(s_brightness_bar, s_brightness * 100 / 255);
            break;
        case 5: /* A - 返回桌面 */
            lv_obj_add_flag(s_settings_root, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);
            desktop_refresh();
            break;
        case 6: /* B - 返回桌面 */
            lv_obj_add_flag(s_settings_root, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(page_get(PAGE_DESKTOP), LV_OBJ_FLAG_HIDDEN);
            desktop_refresh();
            break;
    }
}
