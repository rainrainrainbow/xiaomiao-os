/*
 * ui_desktop.c — Desktop icon grid implementation
 */
#include "ui_desktop.h"
#include "ui_theme.h"
#include "ui_applist.h" /* for app_get_icon_str */
#include <stdio.h>

static void create_icon_cell(ui_desktop_t *desktop, int index, const app_entry_t *app)
{
    lv_obj_t *cell = lv_obj_create(desktop->grid);
    lv_obj_set_size(cell, 48, 48);
    lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_style_pad_all(cell, 2, 0);
    lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);

    /* Icon (icon_str > icon_glyph > first char of name > ?) */
    lv_obj_t *icon_label = lv_label_create(cell);
    lv_label_set_text(icon_label, app_get_icon_str(app));
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(THEME_BLACK), 0);

    /* Name label */
    lv_obj_t *name_label = lv_label_create(cell);
    lv_label_set_text(name_label, app->display_name);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_BLACK), 0);
    lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name_label, 44);

    desktop->icons[index] = cell;
}

static void update_page_dots(ui_desktop_t *desktop)
{
    for (int i = 0; i < 4; i++) {
        if (i < desktop->total_pages) {
            lv_obj_clear_flag(desktop->page_dots[i], LV_OBJ_FLAG_HIDDEN);
            lv_color_t color = (i == desktop->current_page)
                ? lv_color_hex(THEME_CREAM)
                : lv_color_hex(THEME_BROWN);
            lv_obj_set_style_bg_color(desktop->page_dots[i], color, 0);
        } else {
            lv_obj_add_flag(desktop->page_dots[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_selection(ui_desktop_t *desktop)
{
    for (int i = 0; i < UI_ICONS_PER_PAGE; i++) {
        if (desktop->icons[i] == NULL) continue;

        int global_index = desktop->current_page * UI_ICONS_PER_PAGE + i;
        bool is_selected = (global_index == desktop->selected_index);

        lv_color_t bg_color = is_selected ? lv_color_hex(THEME_BROWN) : lv_color_hex(THEME_YELLOW);
        lv_obj_set_style_bg_color(desktop->icons[i], bg_color, 0);
        lv_obj_set_style_bg_opa(desktop->icons[i], is_selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

        lv_obj_t *icon_label = lv_obj_get_child(desktop->icons[i], 0);
        lv_obj_t *name_label = lv_obj_get_child(desktop->icons[i], 1);
        lv_color_t text_color = is_selected ? lv_color_hex(THEME_CREAM) : lv_color_hex(THEME_BLACK);
        lv_obj_set_style_text_color(icon_label, text_color, 0);
        lv_obj_set_style_text_color(name_label, text_color, 0);
    }
}

void ui_desktop_create(ui_desktop_t *desktop, lv_obj_t *parent)
{
    desktop->app_reg = app_manager_get_registry();
    desktop->current_page = 0;
    desktop->selected_index = 0;

    /* Root container */
    desktop->root = lv_obj_create(parent);
    lv_obj_set_size(desktop->root, UI_SCREEN_W, UI_SCREEN_H - UI_STATUSBAR_H);
    lv_obj_set_style_bg_color(desktop->root, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(desktop->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(desktop->root, 0, 0);
    lv_obj_set_style_pad_all(desktop->root, 0, 0);
    lv_obj_set_flex_flow(desktop->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(desktop->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Grid container (3x2) */
    desktop->grid = lv_obj_create(desktop->root);
    lv_obj_set_size(desktop->grid, UI_SCREEN_W, UI_SCREEN_H - UI_STATUSBAR_H - UI_DOCK_H);
    lv_obj_set_style_bg_opa(desktop->grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(desktop->grid, 0, 0);
    lv_obj_set_style_pad_all(desktop->grid, 4, 0);
    lv_obj_set_flex_flow(desktop->grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(desktop->grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(desktop->grid, LV_OBJ_FLAG_SCROLLABLE);

    /* Page indicator dots */
    lv_obj_t *dock = lv_obj_create(desktop->root);
    lv_obj_set_size(dock, UI_SCREEN_W, UI_DOCK_H);
    lv_obj_set_style_bg_opa(dock, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dock, 0, 0);
    lv_obj_set_flex_flow(dock, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dock, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dock, 6, 0);
    lv_obj_clear_flag(dock, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++) {
        desktop->page_dots[i] = lv_obj_create(dock);
        lv_obj_set_size(desktop->page_dots[i], 6, 6);
        lv_obj_set_style_radius(desktop->page_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(desktop->page_dots[i], lv_color_hex(THEME_BROWN), 0);
        lv_obj_set_style_bg_opa(desktop->page_dots[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(desktop->page_dots[i], 0, 0);
        lv_obj_clear_flag(desktop->page_dots[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    ui_desktop_refresh(desktop);
}

void ui_desktop_refresh(ui_desktop_t *desktop)
{
    /* Clear existing icons */
    lv_obj_clean(desktop->grid);

    int total_apps = desktop->app_reg->count;
    desktop->total_pages = (total_apps + UI_ICONS_PER_PAGE - 1) / UI_ICONS_PER_PAGE;
    if (desktop->total_pages == 0) desktop->total_pages = 1;

    if (desktop->current_page >= desktop->total_pages) {
        desktop->current_page = desktop->total_pages - 1;
    }

    int start_index = desktop->current_page * UI_ICONS_PER_PAGE;
    int end_index = start_index + UI_ICONS_PER_PAGE;
    if (end_index > total_apps) end_index = total_apps;

    for (int i = 0; i < UI_ICONS_PER_PAGE; i++) {
        desktop->icons[i] = NULL;
        int global_index = start_index + i;
        if (global_index < end_index) {
            create_icon_cell(desktop, i, &desktop->app_reg->apps[global_index]);
        }
    }

    if (desktop->selected_index >= total_apps) {
        desktop->selected_index = total_apps > 0 ? total_apps - 1 : 0;
    }

    update_page_dots(desktop);
    update_selection(desktop);
}

void ui_desktop_navigate(ui_desktop_t *desktop, int dx, int dy)
{
    int total_apps = desktop->app_reg->count;
    if (total_apps == 0) return;

    int new_index = desktop->selected_index;
    int new_page = desktop->current_page;

    if (dx != 0) {
        int col = new_index % UI_GRID_COLS;
        int new_col = col + dx;

        if (new_col >= 0 && new_col < UI_GRID_COLS) {
            new_index = new_index - col + new_col;
        } else if (new_col < 0 && new_page > 0) {
            new_page--;
            new_index = new_page * UI_ICONS_PER_PAGE + (UI_GRID_COLS - 1);
            if (new_index >= total_apps) new_index = total_apps - 1;
        } else if (new_col >= UI_GRID_COLS && new_page < desktop->total_pages - 1) {
            new_page++;
            new_index = new_page * UI_ICONS_PER_PAGE;
        }
    }

    if (dy != 0) {
        int row = new_index / UI_GRID_COLS;
        int col = new_index % UI_GRID_COLS;
        int new_row = row + dy;

        if (new_row >= 0 && new_row < UI_GRID_ROWS) {
            new_index = new_row * UI_GRID_COLS + col;
        }
    }

    if (new_index < 0) new_index = 0;
    if (new_index >= total_apps) new_index = total_apps - 1;

    new_page = new_index / UI_ICONS_PER_PAGE;

    bool page_changed = (new_page != desktop->current_page);
    bool selection_changed = (new_index != desktop->selected_index);

    desktop->current_page = new_page;
    desktop->selected_index = new_index;

    if (page_changed) {
        ui_desktop_refresh(desktop);
    } else if (selection_changed) {
        update_selection(desktop);
    }
}

void ui_desktop_select(ui_desktop_t *desktop)
{
    /* No-op: handled by caller (ui_main.c). */
}

int ui_desktop_get_selected_app_index(ui_desktop_t *desktop)
{
    return desktop->selected_index;
}