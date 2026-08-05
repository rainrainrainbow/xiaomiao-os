/*
 * ui_appstore.c — Application store page UI implementation
 */
#include "ui_appstore.h"
#include "ui_theme.h"
#include "ui_applist.h"  /* for app_get_icon_str */
#include "esp_log.h"
#include <string.h>

static const char *TAG = "UI_STORE";

static void update_selection(ui_appstore_t *store)
{
    int count = app_store_get_count();
    for (int i = 0; i < count; i++) {
        if (store->items[i] == NULL) continue;

        bool is_selected = (i == store->selected_index);
        lv_color_t bg_color = is_selected ? lv_color_hex(THEME_BROWN) : lv_color_hex(THEME_YELLOW);
        lv_color_t text_color = is_selected ? lv_color_hex(THEME_CREAM) : lv_color_hex(THEME_BLACK);

        lv_obj_set_style_bg_color(store->items[i], bg_color, 0);
        lv_obj_set_style_bg_opa(store->items[i], LV_OPA_COVER, 0);

        lv_obj_t *icon_label = lv_obj_get_child(store->items[i], 0);
        lv_obj_t *name_label = lv_obj_get_child(store->items[i], 1);
        lv_obj_t *status_label = lv_obj_get_child(store->items[i], 2);

        lv_obj_set_style_text_color(icon_label, text_color, 0);
        lv_obj_set_style_text_color(name_label, text_color, 0);
        lv_obj_set_style_text_color(status_label, text_color, 0);
    }
}

void ui_appstore_create(ui_appstore_t *store, lv_obj_t *parent)
{
    store->selected_index = 0;
    memset(store->items, 0, sizeof(store->items));

    /* Root container */
    store->root = lv_obj_create(parent);
    lv_obj_set_size(store->root, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_style_bg_color(store->root, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(store->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(store->root, 0, 0);
    lv_obj_set_style_pad_all(store->root, 0, 0);
    lv_obj_set_flex_flow(store->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(store->root, LV_OBJ_FLAG_SCROLLABLE);

    /* Title bar */
    lv_obj_t *titlebar = lv_obj_create(store->root);
    lv_obj_set_size(titlebar, UI_SCREEN_W, UI_TITLEBAR_H);
    lv_obj_set_style_bg_color(titlebar, lv_color_hex(THEME_BROWN), 0);
    lv_obj_set_style_bg_opa(titlebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(titlebar, 0, 0);
    lv_obj_set_style_pad_all(titlebar, 2, 0);
    lv_obj_clear_flag(titlebar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = lv_label_create(titlebar);
    lv_label_set_text(title_label, "App Store");
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_CREAM), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_10, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 4, 0);

    /* Scrollable list area */
    store->list = lv_obj_create(store->root);
    lv_obj_set_size(store->list, UI_SCREEN_W, UI_SCREEN_H - UI_TITLEBAR_H);
    lv_obj_set_style_bg_color(store->list, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(store->list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(store->list, 0, 0);
    lv_obj_set_style_pad_all(store->list, 2, 0);
    lv_obj_set_flex_flow(store->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(store->list, 2, 0);
    lv_obj_add_flag(store->list, LV_OBJ_FLAG_SCROLLABLE);

    ui_appstore_refresh(store);
}

void ui_appstore_refresh(ui_appstore_t *store)
{
    /* Clear existing items */
    lv_obj_clean(store->list);
    memset(store->items, 0, sizeof(store->items));

    int count = app_store_get_count();
    if (count == 0) {
        /* Show empty message */
        lv_obj_t *empty = lv_label_create(store->list);
        lv_label_set_text(empty, "No apps available");
        lv_obj_set_style_text_color(empty, lv_color_hex(THEME_BROWN), 0);
        lv_obj_set_style_text_font(empty, &lv_font_montserrat_10, 0);
        lv_obj_align(empty, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    /* Create list items */
    for (int i = 0; i < count; i++) {
        const store_item_t *item = app_store_get_item(i);
        if (!item || !item->valid) continue;

        lv_obj_t *row = lv_obj_create(store->list);
        lv_obj_set_size(row, UI_SCREEN_W - 4, 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        /* Icon */
        lv_obj_t *icon_label = lv_label_create(row);
        lv_label_set_text(icon_label, app_get_icon_str((const app_entry_t *)item));
        lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(icon_label, lv_color_hex(THEME_BLACK), 0);

        /* Name */
        lv_obj_t *name_label = lv_label_create(row);
        lv_label_set_text(name_label, item->display_name);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_BLACK), 0);
        lv_label_set_long_mode(name_label, LV_LABEL_LONG_DOT);
        lv_obj_set_flex_grow(name_label, 1);

        /* Status (Installed / Available) */
        lv_obj_t *status_label = lv_label_create(row);
        lv_label_set_text(status_label, item->installed ? "Installed" : "Install");
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(status_label, lv_color_hex(THEME_BLACK), 0);

        store->items[i] = row;
    }

    /* Ensure selected_index is valid */
    if (store->selected_index >= count) {
        store->selected_index = count > 0 ? count - 1 : 0;
    }

    update_selection(store);
}

void ui_appstore_navigate(ui_appstore_t *store, int dy)
{
    int count = app_store_get_count();
    if (count == 0) return;

    int new_index = store->selected_index + dy;
    if (new_index < 0) new_index = 0;
    if (new_index >= count) new_index = count - 1;

    if (new_index != store->selected_index) {
        store->selected_index = new_index;
        update_selection(store);

        if (store->items[new_index] != NULL) {
            lv_obj_scroll_to_view(store->items[new_index], LV_ANIM_ON);
        }
    }
}

int ui_appstore_get_selected_index(ui_appstore_t *store)
{
    return store->selected_index;
}

void ui_appstore_install_selected(ui_appstore_t *store)
{
    int idx = store->selected_index;
    const store_item_t *item = app_store_get_item(idx);
    if (!item || !item->valid) return;

    if (item->installed) {
        ESP_LOGI(TAG, "App already installed: %s", item->display_name);
        return;
    }

    ESP_LOGI(TAG, "Installing: %s", item->display_name);
    bool ok = app_store_install(idx);
    if (ok) {
        ESP_LOGI(TAG, "Install success: %s", item->display_name);
        /* Refresh UI to show "Installed" status */
        ui_appstore_refresh(store);
    } else {
        ESP_LOGE(TAG, "Install failed: %s", item->display_name);
    }
}