/*
 * ui_applist.h — Application list view
 */
#pragma once
#include "lvgl.h"
#include "../app_manager.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *list;
    lv_obj_t *items[APP_MAX_COUNT];
    int selected_index;
    app_registry_t *app_reg;
} ui_applist_t;

void ui_applist_create(ui_applist_t *applist, lv_obj_t *parent);
void ui_applist_refresh(ui_applist_t *applist);
void ui_applist_navigate(ui_applist_t *applist, int dy);
int  ui_applist_get_selected_app_index(ui_applist_t *applist);

/* Render helper: get the best icon string for an app */
const char *app_get_icon_str(const app_entry_t *app);