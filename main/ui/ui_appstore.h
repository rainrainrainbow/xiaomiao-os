/*
 * ui_appstore.h — Application store page UI
 */
#pragma once
#include "lvgl.h"
#include "../app_store.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *list;
    lv_obj_t *items[STORE_MAX_ITEMS];
    int selected_index;
} ui_appstore_t;

void ui_appstore_create(ui_appstore_t *store, lv_obj_t *parent);
void ui_appstore_refresh(ui_appstore_t *store);
void ui_appstore_navigate(ui_appstore_t *store, int dy);
int  ui_appstore_get_selected_index(ui_appstore_t *store);
void ui_appstore_install_selected(ui_appstore_t *store);