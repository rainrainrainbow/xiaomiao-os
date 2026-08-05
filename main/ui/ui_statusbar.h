/*
 * ui_statusbar.h — Top status bar (time + battery)
 * Height: 12px, brown background, cream text
 */
#pragma once
#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *time_label;
    lv_obj_t *batt_bar;
    lv_obj_t *batt_fill;
} ui_statusbar_t;

void ui_statusbar_create(ui_statusbar_t *sb, lv_obj_t *parent);
void ui_statusbar_update_time(ui_statusbar_t *sb);
void ui_statusbar_update_battery(ui_statusbar_t *sb, float voltage);