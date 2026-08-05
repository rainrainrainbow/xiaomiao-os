/*
 * ui_main.h — Main UI manager (page switching & key event dispatch)
 */
#pragma once
#include "lvgl.h"
#include "../app_manager.h"
#include "ui_statusbar.h"
#include "ui_desktop.h"
#include "ui_applist.h"
#include "ui_settings.h"
#include "ui_apprun.h"
#include "ui_appstore.h"

typedef enum {
    PAGE_DESKTOP,
    PAGE_APPLIST,
    PAGE_SETTINGS,
    PAGE_APPRUN,
    PAGE_APPSTORE,
} ui_page_t;

typedef struct {
    lv_obj_t *screens[5];  /* One screen object per page */
    ui_page_t current_page;

    /* Page components */
    ui_statusbar_t statusbar;
    ui_desktop_t desktop;
    ui_applist_t applist;
    ui_settings_t settings;
    ui_apprun_t apprun;
    ui_appstore_t appstore;

    /* Settings data (loaded from /vfs/settings.json by app_manager/settings modules) */
    bool wifi_enabled;
    int brightness;   /* 0..255 */
    int volume;       /* 0..100 */
} ui_manager_t;

void ui_manager_init(ui_manager_t *ui);
void ui_manager_navigate(ui_manager_t *ui, ui_page_t page);
void ui_manager_handle_key(ui_manager_t *ui, uint32_t key, bool pressed);
void ui_manager_update_time(ui_manager_t *ui);
void ui_manager_update_battery(ui_manager_t *ui, float voltage);
void ui_manager_launch_app(ui_manager_t *ui, int app_index);
void ui_manager_refresh_app_views(ui_manager_t *ui); /* re-render desktop + applist after registry change */