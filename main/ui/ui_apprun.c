/*
 * ui_apprun.c — App running view implementation
 */
#include "ui_apprun.h"
#include "ui_theme.h"

void ui_apprun_create(ui_apprun_t *apprun, lv_obj_t *parent)
{
    /* Root container */
    apprun->root = lv_obj_create(parent);
    lv_obj_set_size(apprun->root, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_style_bg_color(apprun->root, lv_color_hex(THEME_YELLOW), 0);
    lv_obj_set_style_bg_opa(apprun->root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(apprun->root, 0, 0);
    lv_obj_set_style_pad_all(apprun->root, 0, 0);
    lv_obj_set_flex_flow(apprun->root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(apprun->root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(apprun->root, 8, 0);
    lv_obj_clear_flag(apprun->root, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Icon label (large) */
    apprun->icon_label = lv_label_create(apprun->root);
    lv_label_set_text(apprun->icon_label, "?");
    lv_obj_set_style_text_font(apprun->icon_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(apprun->icon_label, lv_color_hex(THEME_BLACK), 0);
    
    /* Name label */
    apprun->name_label = lv_label_create(apprun->root);
    lv_label_set_text(apprun->name_label, "App");
    lv_obj_set_style_text_font(apprun->name_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(apprun->name_label, lv_color_hex(THEME_BLACK), 0);
    
    /* Status label */
    apprun->status_label = lv_label_create(apprun->root);
    lv_label_set_text(apprun->status_label, "Running...");
    lv_obj_set_style_text_font(apprun->status_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(apprun->status_label, lv_color_hex(THEME_BROWN), 0);
}

void ui_apprun_show(ui_apprun_t *apprun, const char *icon, const char *name, const char *status)
{
    lv_label_set_text(apprun->icon_label, icon);
    lv_label_set_text(apprun->name_label, name);
    lv_label_set_text(apprun->status_label, status);
}

void ui_apprun_update_status(ui_apprun_t *apprun, const char *status)
{
    lv_label_set_text(apprun->status_label, status);
}