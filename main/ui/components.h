#pragma once
#include "lvgl.h"

/* 简单按钮（磁贴风格） */
lv_obj_t *ui_button_create(lv_obj_t *parent, const char *text, lv_color_t bg_color,
                           int x, int y, int w, int h);
void      ui_button_set_callback(lv_obj_t *btn, lv_event_cb_t cb);

/* 列表项 */
lv_obj_t *ui_list_item_create(lv_obj_t *parent, const char *text, const char *subtext,
                              lv_color_t accent);
/* 进度条 */
lv_obj_t *ui_progress_create(lv_obj_t *parent, int w, int h);
void      ui_progress_set(lv_obj_t *bar, int value); /* 0-100 */

/* 对话框 */
void      ui_alert(const char *title, const char *msg);
void      ui_confirm(const char *title, const char *msg, lv_event_cb_t on_yes);

/* 图标（用文字替代，节省空间） */
lv_obj_t *ui_icon_create(lv_obj_t *parent, const char *symbol, lv_color_t color, int size);

/* 标签快捷创建 */
lv_obj_t *ui_label_create(lv_obj_t *parent, const char *text, lv_color_t color,
                          const lv_font_t *font, lv_align_t align, int x, int y);
