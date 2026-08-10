// ================ toast.c - Toast 提示 ================

#include "toast.h"
#include "theme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static lv_obj_t *s_toast = NULL;
static lv_timer_t *s_timer = NULL;

static void hide_cb(lv_timer_t *t)
{
    (void)t;
    if (s_toast) lv_obj_set_style_opa(s_toast, LV_OPA_TRANSP, 0);
}

void toast_init(lv_obj_t *parent)
{
    s_toast = lv_obj_create(parent);
    lv_obj_set_size(s_toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(s_toast, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_style_bg_color(s_toast, C_BLACK, 0);
    lv_obj_set_style_text_color(s_toast, C_CREAM, 0);
    lv_obj_set_style_radius(s_toast, 4, 0);
    lv_obj_set_style_pad_hor(s_toast, 6, 0);
    lv_obj_set_style_pad_ver(s_toast, 2, 0);
    lv_obj_set_style_opa(s_toast, LV_OPA_TRANSP, 0);

    lv_obj_t *label = lv_label_create(s_toast);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_8, 0);
    lv_label_set_text(label, "");
    lv_obj_set_user_data(s_toast, label);  // LVGL 9: setter 替代直接字段访问
}

void toast_show(const char *text, uint32_t duration_ms)
{
    if (!s_toast) return;
    lv_obj_t *label = (lv_obj_t *)lv_obj_get_user_data(s_toast);
    lv_label_set_text(label, text);
    lv_obj_set_style_opa(s_toast, (lv_opa_t)LV_OPA_90, 0);

    if (s_timer) lv_timer_delete(s_timer);
    s_timer = lv_timer_create(hide_cb, duration_ms, NULL);
    lv_timer_set_repeat_count(s_timer, 1);
}