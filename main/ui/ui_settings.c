/*
 * ui_settings.c — Settings page implementation
 */
#include "ui_settings.h"
#include "ui_theme.h"
#include <stdio.h>

static const settings_item_t *s_items = NULL;
static int s_item_count = 0;

static void update_selection(ui_settings_t *settings)
{
    for (int i = 0; i < settings->item_count; i++) {
        if (settings->items[i] == NULL) continue;
        
        bool is_selected = (i == settings->selected_index);
        lv_color_t bg_color = is_selected ? lv_color_hex(THEME_BROWN) : lv_color_hex(THEME_YELLOW);
        lv_color_t text_color = is_selected ? lv_color_hex(THEME_CREAM) : lv_color_hex(THEME_BLACK);
        
        lv_obj_set_style_bg_color(settings->items[i], bg_color, 0);
        lv_obj_set_style_bg_opa(settings->items[i], LV_OPA_COVER, 0);
        
        /* Update text colors for children */
        lv_obj_t *label = lv_obj_get_child(settings->items[i], 0);
        lv_obj_t *value = lv_obj_get_child(settings->items[i], 1);
        
        lv_obj_set_style_text_color(label, text_color, 0);
        lv_obj_set_style_text_color(value, text_color, 0);
    }
}

static void update_value_display(ui_settings_t *settings, int index)
{
    if (index < 0 || index >= s_item_count) return;
    if (settings->items[index] == NULL) return;
    
    const settings_item_t *item = &s_items[index];
    lv_obj_t *value_label = lv_obj_get_child(settings->items[index], 1);
    
    char buf[32];
    switch (item->type) {
        case SETTINGS_TOGGLE:
            lv_label_set_text(value_label, *item->toggle_value ? "ON" : "OFF");
            break;
        case SETTINGS_VALUE:
            snprintf(buf, sizeof(buf), "%d%s", *item->numeric_value, item->unit ? item->unit : "");
            lv_label_set_text(value_label, buf);
            break;
        case SETTINGS_ACTION:
        default:
            break;
    }
}

void ui_settings_create(ui_settings_t *settings, lv_obj_t *parent)
{
    settings->selected_index = 0;
    settings->item_count = 0;
    
    /* Root container */
    settings->root = lv_obj_create(parent);
    lv_obj_set_size(settings->root, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_style_bg_color(settings->root, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(settings->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(settings->root, 0, 0);
    lv_obj_set_style_pad_all(settings->root, 0, 0);
    lv_obj_set_flex_flow(settings->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(settings->root, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Title bar */
    lv_obj_t *titlebar = lv_obj_create(settings->root);
    lv_obj_set_size(titlebar, UI_SCREEN_W, UI_TITLEBAR_H);
    lv_obj_set_style_bg_color(titlebar, lv_color_hex(THEME_BROWN), 0);
    lv_obj_set_style_bg_opa(titlebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(titlebar, 0, 0);
    lv_obj_set_style_pad_all(titlebar, 2, 0);
    lv_obj_clear_flag(titlebar, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *title_label = lv_label_create(titlebar);
    lv_label_set_text(title_label, "Settings");
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_CREAM), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_10, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 4, 0);
    
    /* Scrollable list area */
    settings->list = lv_obj_create(settings->root);
    lv_obj_set_size(settings->list, UI_SCREEN_W, UI_SCREEN_H - UI_TITLEBAR_H);
    lv_obj_set_style_bg_color(settings->list, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(settings->list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(settings->list, 0, 0);
    lv_obj_set_style_pad_all(settings->list, 2, 0);
    lv_obj_set_flex_flow(settings->list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(settings->list, 2, 0);
    lv_obj_add_flag(settings->list, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_settings_set_items(ui_settings_t *settings, const settings_item_t *items, int count)
{
    s_items = items;
    s_item_count = count;
    settings->item_count = count;
    
    /* Clear existing items */
    lv_obj_clean(settings->list);
    
    /* Create list items */
    for (int i = 0; i < count && i < 8; i++) {
        const settings_item_t *item = &items[i];
        
        lv_obj_t *row = lv_obj_create(settings->list);
        lv_obj_set_size(row, UI_SCREEN_W - 4, 16);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 2, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        
        /* Label */
        lv_obj_t *label = lv_label_create(row);
        lv_label_set_text(label, item->label);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(THEME_BLACK), 0);
        
        /* Value display */
        lv_obj_t *value = lv_label_create(row);
        char buf[32];
        switch (item->type) {
            case SETTINGS_TOGGLE:
                lv_label_set_text(value, *item->toggle_value ? "ON" : "OFF");
                break;
            case SETTINGS_VALUE:
                snprintf(buf, sizeof(buf), "%d%s", *item->numeric_value, item->unit ? item->unit : "");
                lv_label_set_text(value, buf);
                break;
            case SETTINGS_ACTION:
            default:
                lv_label_set_text(value, ">");
                break;
        }
        lv_obj_set_style_text_font(value, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(value, lv_color_hex(THEME_BLACK), 0);
        
        settings->items[i] = row;
    }
    
    /* Ensure selected_index is valid */
    if (settings->selected_index >= count) {
        settings->selected_index = count > 0 ? count - 1 : 0;
    }
    
    update_selection(settings);
}

void ui_settings_navigate(ui_settings_t *settings, int dy)
{
    int total = settings->item_count;
    if (total == 0) return;
    
    int new_index = settings->selected_index + dy;
    if (new_index < 0) new_index = 0;
    if (new_index >= total) new_index = total - 1;
    
    if (new_index != settings->selected_index) {
        settings->selected_index = new_index;
        update_selection(settings);
        
        /* Scroll to keep selected item visible */
        if (settings->items[new_index] != NULL) {
            lv_obj_scroll_to_view(settings->items[new_index], LV_ANIM_ON);
        }
    }
}

void ui_settings_activate(ui_settings_t *settings)
{
    int index = settings->selected_index;
    if (index < 0 || index >= s_item_count) return;
    
    const settings_item_t *item = &s_items[index];
    
    switch (item->type) {
        case SETTINGS_ACTION:
            if (item->action) item->action();
            break;
        case SETTINGS_TOGGLE:
            if (item->toggle_value) {
                *item->toggle_value = !(*item->toggle_value);
                update_value_display(settings, index);
            }
            break;
        case SETTINGS_VALUE:
            if (item->numeric_value) {
                (*item->numeric_value)++;
                if (*item->numeric_value > 100) *item->numeric_value = 0;
                update_value_display(settings, index);
            }
            break;
    }
}

int ui_settings_get_selected_index(ui_settings_t *settings)
{
    return settings->selected_index;
}