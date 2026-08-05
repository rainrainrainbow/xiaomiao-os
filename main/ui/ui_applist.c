/*
 * ui_applist.c — Application list view implementation
 */
#include "ui_applist.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>

/* ── Helpers ─────────────────────────────────────────────────────────── */

const char *app_get_icon_str(const app_entry_t *app)
{
    if (app == NULL) return "?";
    if (app->icon_str[0] != '\0') return app->icon_str;
    if (app->icon_glyph != 0) {
        static __thread char buf[2];
        buf[0] = (char)app->icon_glyph;
        buf[1] = '\0';
        return buf;
    }
    if (app->display_name[0] != '\0') {
        static __thread char fb[2];
        fb[0] = app->display_name[0];
        fb[1] = '\0';
        return fb;
    }
    return "?";
}

static const char *app_get_subtitle(const app_entry_t *app)
{
    if (app == NULL) return "";
    if (app->is_system) return "System";
    return app->version[0] ? app->version : "User";
}

static void update_selection(ui_applist_t *applist)
{
    for (int i = 0; i < applist->app_reg->count; i++) {
        if (applist->items[i] == NULL) continue;

        bool is_selected = (i == applist->selected_index);
        lv_color_t bg_color = is_selected ? lv_color_hex(THEME_BROWN) : lv_color_hex(THEME_YELLOW);
        lv_color_t text_color = is_selected ? lv_color_hex(THEME_CREAM) : lv_color_hex(THEME_BLACK);

        lv_obj_set_style_bg_color(applist->items[i], bg_color, 0);
        lv_obj_set_style_bg_opa(applist->items[i], LV_OPA_COVER, 0);

        lv_obj_t *icon_label = lv_obj_get_child(applist->items[i], 0);
        lv_obj_t *name_label = lv_obj_get_child(applist->items[i], 1);
        lv_obj_t *ver_label = lv_obj_get_child(applist->items[i], 2);

        lv_obj_set_style_text_color(icon_label, text_color, 0);
        lv_obj_set_style_text_color(name_label, text_color, 0);
        lv_obj_set_style_text_color(ver_label, text_color, 0);
    }
}

/* ── Public API ──────────────────────────────────────────────────────── */

void ui_applist_create(ui_applist_t *applist, lv_obj_t *parent)
{
    applist->app_reg = app_manager_get_registry();
    applist->selected_index = 0;
    memset(applist->items, 0, sizeof(applist->items));

    /* Root container */
    applist->root = lv_obj_create(parent);
    lv_obj_set_size(applist->root, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_style_bg_color(applist->root, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(applist->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(applist->root, 0, 0);
    lv_obj_set_style_pad_all(applist->root, 0, 0);
    lv_obj_set_flex_flow(applist->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(applist->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Title bar */
    lv_obj_t *titlebar = lv_obj_create(applist->root);
    lv_obj_set_size(titlebar, UI_SCREEN_W, UI_TITLEBAR_H);
    lv_obj_set_style_bg_color(titlebar, lv_color_hex(THEME_BROWN), 0);
    lv_obj_set_style_bg_opa(titlebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(titlebar, 0, 0);
    lv_obj_set_style_pad_all(titlebar, 2, 0);
    lv_obj_clear_flag(titlebar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = lv_label_create(titlebar);
    lv_label_set_text(title_label, "Apps");
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_CREAM), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_10, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 4, 0);

    /* Scrollable list area */
    applist->list = lv_obj_create(applist->root);
    lv_obj_set_size(applist->list, UI_SCREEN_W, UI_SCREEN_H - UI_TITLEBAR_H);
    lv_obj_set_style_bg_color(applist->list, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(applist->list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(applist->list, 0, 0);
    lv_obj_set_style_pad_all(applist->list, 2, 0);
    lv_obj_set_flex_flow(applist->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(applist->list, 2, 0);
    lv_obj_add_flag(applist->list, LV_OBJ_FLAG_SCROLLABLE);

    ui_applist_refresh(applist);
}

void ui_applist_refresh(ui_applist_t *applist)
{
    /* Clear existing items */
    lv_obj_clean(applist->list);
    memset(applist->items, 0, sizeof(applist->items));

    /* Create list items */
    for (int i = 0; i < applist->app_reg->count; i++) {
        const app_entry_t *app = &applist->app_reg->apps[i];

        lv_obj_t *item = lv_obj_create(applist->list);
        lv_obj_set_size(item, UI_SCREEN_W - 4, 16);
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_pad_all(item, 2, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(item, 4, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

        /* Icon */
        lv_obj_t *icon_label = lv_label_create(item);
        lv_label_set_text(icon_label, app_get_icon_str(app));
        lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(icon_label, lv_color_hex(THEME_BLACK), 0);

        /* Name */
        lv_obj_t *name_label = lv_label_create(item);
        lv_label_set_text(name_label, app->display_name);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_BLACK), 0);
        lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
        lv_obj_set_flex_grow(name_label, 1);

        /* Subtitle (version / System) */
        lv_obj_t *ver_label = lv_label_create(item);
        lv_label_set_text(ver_label, app_get_subtitle(app));
        lv_obj_set_style_text_font(ver_label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(ver_label, lv_color_hex(THEME_BLACK), 0);

        applist->items[i] = item;
    }

    /* Ensure selected_index is valid */
    if (applist->selected_index >= applist->app_reg->count) {
        applist->selected_index = applist->app_reg->count > 0 ? applist->app_reg->count - 1 : 0;
    }

    update_selection(applist);
}

void ui_applist_navigate(ui_applist_t *applist, int dy)
{
    int total = applist->app_reg->count;
    if (total == 0) return;

    int new_index = applist->selected_index + dy;
    if (new_index < 0) new_index = 0;
    if (new_index >= total) new_index = total - 1;

    if (new_index != applist->selected_index) {
        applist->selected_index = new_index;
        update_selection(applist);

        if (applist->items[new_index] != NULL) {
            lv_obj_scroll_to_view(applist->items[new_index], LV_ANIM_ON);
        }
    }
}

int ui_applist_get_selected_app_index(ui_applist_t *applist)
{
    return applist->selected_index;
}