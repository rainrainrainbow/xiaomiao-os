#pragma once
#include "lvgl.h"

/* 高级控件 */

/* 导航栏 (底部) */
lv_obj_t *widgets_navbar_create(lv_obj_t *parent);
void      widgets_navbar_set(lv_obj_t *bar, const char *left, const char *center, const char *right);

/* 弹出菜单 */
lv_obj_t *widgets_popup_create(const char *title);
void      widgets_popup_add_item(lv_obj_t *popup, const char *text, lv_event_cb_t cb);
void      widgets_popup_close(lv_obj_t *popup);

/* 滑动条 (设置用) */
lv_obj_t *widgets_slider_create(lv_obj_t *parent, int x, int y, int w, int min, int max, int val);
void      widgets_slider_set(lv_obj_t *slider, int val);
int       widgets_slider_get(lv_obj_t *slider);

/* 开关 */
lv_obj_t *widgets_toggle_create(lv_obj_t *parent, int x, int y, bool on);
bool      widgets_toggle_get(lv_obj_t *toggle);

/* 图标按钮 (带符号文字) */
lv_obj_t *widgets_icon_btn_create(lv_obj_t *parent, const char *symbol, lv_color_t bg, int x, int y, int size);
