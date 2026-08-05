/*
 * ui_apprun.h — App running view (displays when an app is launched)
 */
#pragma once
#include "lvgl.h"

typedef struct {
    lv_obj_t *root;
    lv_obj_t *icon_label;
    lv_obj_t *name_label;
    lv_obj_t *status_label;
} ui_apprun_t;

void ui_apprun_create(ui_apprun_t *apprun, lv_obj_t *parent);
void ui_apprun_show(ui_apprun_t *apprun, const char *icon, const char *name, const char *status);
void ui_apprun_update_status(ui_apprun_t *apprun, const char *status);