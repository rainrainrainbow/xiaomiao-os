/*
 * ui_desktop.h — Desktop icon grid (3x2 layout)
 * Displays app icons with page indicator dots
 */
#pragma once
#include "lvgl.h"
#include "../app_manager.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *grid;
    lv_obj_t *icons[UI_ICONS_PER_PAGE];
    lv_obj_t *page_dots[4];  /* max 4 pages */
    int current_page;
    int total_pages;
    int selected_index;
    app_registry_t *app_reg;
} ui_desktop_t;

void ui_desktop_create(ui_desktop_t *desktop, lv_obj_t *parent);
void ui_desktop_refresh(ui_desktop_t *desktop);
void ui_desktop_navigate(ui_desktop_t *desktop, int dx, int dy);
void ui_desktop_select(ui_desktop_t *desktop);
int  ui_desktop_get_selected_app_index(ui_desktop_t *desktop);