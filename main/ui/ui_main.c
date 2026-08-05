/*
 * ui_main.c — Main UI manager implementation
 */
#include "ui_main.h"
#include "ui_theme.h"
#include "ui_applist.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "UI";

/* ── Settings action handlers ─────────────────────────────────────────── */

static ui_manager_t *s_ui = NULL;

static void settings_action_about(void)
{
    ESP_LOGI(TAG, "About: XiaoMiao OS v0.1.0");
    /* TODO: show about screen */
}

static void settings_action_store(void)
{
    /* Navigate to the app store page */
    if (s_ui) ui_manager_navigate(s_ui, PAGE_APPSTORE);
}

/* ── Settings items configuration ─────────────────────────────────────── */

static const settings_item_t s_settings_items[] = {
    { .label = "WiFi",      .type = SETTINGS_TOGGLE, .toggle_value = NULL },
    { .label = "Brightness",.type = SETTINGS_VALUE,  .numeric_value = NULL, .unit = "%" },
    { .label = "Volume",    .type = SETTINGS_VALUE,  .numeric_value = NULL, .unit = "%" },
    { .label = "App Store", .type = SETTINGS_ACTION, .action = settings_action_store },
    { .label = "About",     .type = SETTINGS_ACTION, .action = settings_action_about },
};
#define NUM_SETTINGS_ITEMS (sizeof(s_settings_items) / sizeof(s_settings_items[0]))

/* ── Page creation ────────────────────────────────────────────────────── */

static void create_desktop_page(ui_manager_t *ui)
{
    ui->screens[PAGE_DESKTOP] = lv_obj_create(NULL);
    lv_obj_t *scr = ui->screens[PAGE_DESKTOP];
    lv_obj_set_style_bg_color(scr, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui_statusbar_create(&ui->statusbar, scr);
    ui_desktop_create(&ui->desktop, scr);
    lv_obj_set_pos(ui->desktop.root, 0, UI_STATUSBAR_H);
}

static void create_applist_page(ui_manager_t *ui)
{
    ui->screens[PAGE_APPLIST] = lv_obj_create(NULL);
    lv_obj_t *scr = ui->screens[PAGE_APPLIST];
    lv_obj_set_style_bg_color(scr, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui_applist_create(&ui->applist, scr);
}

static void create_settings_page(ui_manager_t *ui)
{
    ui->screens[PAGE_SETTINGS] = lv_obj_create(NULL);
    lv_obj_t *scr = ui->screens[PAGE_SETTINGS];
    lv_obj_set_style_bg_color(scr, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui_settings_create(&ui->settings, scr);

    settings_item_t items[NUM_SETTINGS_ITEMS];
    memcpy(items, s_settings_items, sizeof(s_settings_items));
    items[0].toggle_value  = &ui->wifi_enabled;
    items[1].numeric_value = &ui->brightness;
    items[2].numeric_value = &ui->volume;
    ui_settings_set_items(&ui->settings, items, NUM_SETTINGS_ITEMS);
}

static void create_apprun_page(ui_manager_t *ui)
{
    ui->screens[PAGE_APPRUN] = lv_obj_create(NULL);
    lv_obj_t *scr = ui->screens[PAGE_APPRUN];
    lv_obj_set_style_bg_color(scr, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui_apprun_create(&ui->apprun, scr);
}

static void create_appstore_page(ui_manager_t *ui)
{
    ui->screens[PAGE_APPSTORE] = lv_obj_create(NULL);
    lv_obj_t *scr = ui->screens[PAGE_APPSTORE];
    lv_obj_set_style_bg_color(scr, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    ui_appstore_create(&ui->appstore, scr);
}

/* ── Public API ───────────────────────────────────────────────────────── */

void ui_manager_init(ui_manager_t *ui)
{
    s_ui = ui;
    ui->current_page = PAGE_DESKTOP;

    ui->wifi_enabled = false;
    ui->brightness   = 80;
    ui->volume       = 50;

    create_desktop_page(ui);
    create_applist_page(ui);
    create_settings_page(ui);
    create_apprun_page(ui);
    create_appstore_page(ui);

    lv_scr_load(ui->screens[PAGE_DESKTOP]);
    ESP_LOGI(TAG, "UI manager initialized");
}

void ui_manager_navigate(ui_manager_t *ui, ui_page_t page)
{
    if (page == ui->current_page) return;
    lv_scr_load_anim(ui->screens[page], LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
    ui->current_page = page;
    ESP_LOGI(TAG, "Navigated to page %d", page);
}

void ui_manager_refresh_app_views(ui_manager_t *ui)
{
    ui_desktop_refresh(&ui->desktop);
    ui_applist_refresh(&ui->applist);
}

void ui_manager_handle_key(ui_manager_t *ui, uint32_t key, bool pressed)
{
    if (!pressed) return;

    switch (ui->current_page) {
        case PAGE_DESKTOP:
            if      (key == LV_KEY_UP)    ui_desktop_navigate(&ui->desktop,  0, -1);
            else if (key == LV_KEY_DOWN)  ui_desktop_navigate(&ui->desktop,  0,  1);
            else if (key == LV_KEY_LEFT)  ui_desktop_navigate(&ui->desktop, -1,  0);
            else if (key == LV_KEY_RIGHT) ui_desktop_navigate(&ui->desktop,  1,  0);
            else if (key == LV_KEY_ENTER) ui_manager_launch_app(ui, ui_desktop_get_selected_app_index(&ui->desktop));
            break;

        case PAGE_APPLIST:
            if      (key == LV_KEY_UP)    ui_applist_navigate(&ui->applist, -1);
            else if (key == LV_KEY_DOWN)  ui_applist_navigate(&ui->applist,  1);
            else if (key == LV_KEY_ENTER) ui_manager_launch_app(ui, ui_applist_get_selected_app_index(&ui->applist));
            else if (key == LV_KEY_ESC)   ui_manager_navigate(ui, PAGE_DESKTOP);
            break;

        case PAGE_SETTINGS:
            if      (key == LV_KEY_UP)    ui_settings_navigate(&ui->settings, -1);
            else if (key == LV_KEY_DOWN)  ui_settings_navigate(&ui->settings,  1);
            else if (key == LV_KEY_ENTER) ui_settings_activate(&ui->settings);
            else if (key == LV_KEY_ESC)   ui_manager_navigate(ui, PAGE_DESKTOP);
            break;

        case PAGE_APPRUN:
            if (key == LV_KEY_ESC) ui_manager_navigate(ui, PAGE_DESKTOP);
            break;

        case PAGE_APPSTORE:
            if      (key == LV_KEY_UP)    ui_appstore_navigate(&ui->appstore, -1);
            else if (key == LV_KEY_DOWN)  ui_appstore_navigate(&ui->appstore,  1);
            else if (key == LV_KEY_ENTER) ui_appstore_install_selected(&ui->appstore);
            else if (key == LV_KEY_ESC)   ui_manager_navigate(ui, PAGE_DESKTOP);
            break;
    }
}

void ui_manager_update_time(ui_manager_t *ui)
{
    ui_statusbar_update_time(&ui->statusbar);
}

void ui_manager_update_battery(ui_manager_t *ui, float voltage)
{
    ui_statusbar_update_battery(&ui->statusbar, voltage);
}

void ui_manager_launch_app(ui_manager_t *ui, int app_index)
{
    const app_entry_t *app = app_manager_get(app_index);
    if (!app) return;

    /* System app routing */
    if (app->is_system) {
        if (strcmp(app->package_name, "sys.applist") == 0) {
            ui_manager_navigate(ui, PAGE_APPLIST);
            return;
        }
        if (strcmp(app->package_name, "sys.settings") == 0) {
            ui_manager_navigate(ui, PAGE_SETTINGS);
            return;
        }
        if (strcmp(app->package_name, "sys.store") == 0) {
            ui_manager_navigate(ui, PAGE_APPSTORE);
            return;
        }
        if (strcmp(app->package_name, "sys.editor") == 0) {
            /* TODO: open block editor page; for now show apprun placeholder */
            ui_apprun_show(&ui->apprun, "E", app->display_name, "Editor (PoC)");
            ui_manager_navigate(ui, PAGE_APPRUN);
            return;
        }
        if (strcmp(app->package_name, "sys.about") == 0) {
            ui_apprun_show(&ui->apprun, "i", app->display_name, "XiaoMiao OS v0.1.0");
            ui_manager_navigate(ui, PAGE_APPRUN);
            return;
        }
    }

    /* User app: hand off to MicroPython */
    char status[64];
    snprintf(status, sizeof(status), "Launching %s...", app->display_name);
    ui_apprun_show(&ui->apprun, app_get_icon_str(app), app->display_name, status);
    ui_manager_navigate(ui, PAGE_APPRUN);

    app_manager_launch_index(app_index);
    ui_apprun_update_status(&ui->apprun, "Running");
}