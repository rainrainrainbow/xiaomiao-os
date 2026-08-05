/*
 * ui_settings.h — Settings page
 */
#pragma once
#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *list;
    lv_obj_t *items[8];  /* Max 8 settings */
    int selected_index;
    int item_count;
} ui_settings_t;

/* Settings item types */
typedef enum {
    SETTINGS_ACTION,    /* Simple action (e.g., "About") */
    SETTINGS_TOGGLE,    /* Boolean toggle */
    SETTINGS_VALUE,     /* Numeric value with +/- */
} settings_type_t;

typedef struct {
    const char *label;
    settings_type_t type;
    union {
        void (*action)(void);
        bool *toggle_value;
        int *numeric_value;
    };
    const char *unit;  /* For numeric values */
} settings_item_t;

void ui_settings_create(ui_settings_t *settings, lv_obj_t *parent);
void ui_settings_set_items(ui_settings_t *settings, const settings_item_t *items, int count);
void ui_settings_navigate(ui_settings_t *settings, int dy);
void ui_settings_activate(ui_settings_t *settings);
int  ui_settings_get_selected_index(ui_settings_t *settings);