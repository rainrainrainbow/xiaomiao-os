// ================ canvas.c - MicroPython 画布 + 按键桥接 ================

#include "canvas.h"
#include "hal/keys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "canvas";

static lv_obj_t *s_canvas = NULL;
static lv_color_t *s_buf = NULL;   // 160x128 RGBA? - LVGL canvas buf
static bool s_active = false;

// ================ 创建/销毁 ================
esp_err_t canvas_create(void)
{
    if (s_canvas) return ESP_OK;

    s_buf = heap_caps_malloc(160 * 128 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!s_buf) return ESP_ERR_NO_MEM;
    memset(s_buf, 0, 160 * 128 * sizeof(lv_color_t));

    s_canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(s_canvas, s_buf, 160, 128, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_size(s_canvas, 160, 128);
    lv_obj_set_pos(s_canvas, 0, 0);
    s_active = true;
    ESP_LOGI(TAG, "canvas created");
    return ESP_OK;
}

void canvas_destroy(void)
{
    if (!s_canvas) return;
    lv_obj_delete(s_canvas);
    s_canvas = NULL;
    if (s_buf) {
        free(s_buf);
        s_buf = NULL;
    }
    s_active = false;
    ESP_LOGI(TAG, "canvas destroyed");
}

// ================ 绘制 API ================
void canvas_fill(uint16_t color)
{
    if (!s_active || !s_canvas) return;
    lv_canvas_fill_bg(s_canvas, lv_color_make(((color >> 11) & 0x1F) << 3,
                                              ((color >> 5) & 0x3F) << 2,
                                              (color & 0x1F) << 3),
                      LV_OPA_COVER);
}

void canvas_set_pixel(int x, int y, uint16_t color)
{
    if (!s_active || !s_canvas) return;
    if (x < 0 || y < 0 || x >= 160 || y >= 128) return;
    lv_canvas_set_px(s_canvas, x, y,
                     lv_color_make(((color >> 11) & 0x1F) << 3,
                                   ((color >> 5) & 0x3F) << 2,
                                   (color & 0x1F) << 3),
                     LV_OPA_COVER);
}

void canvas_text(int x, int y, const char *str)
{
    if (!s_active || !s_canvas || !str) return;
    // 用 layer 在画布上叠一个 label (简化)
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = lv_color_black();
    dsc.font = &lv_font_montserrat_8;

    // LVGL 9 在 obj 上画 label: 直接 create 一临时 label
    // 此处用更轻量办法: 通过 canvas 的 fill + 自定义层叠文字 (占位)
    // 真实做法是 lv_canvas 内部存 buffer, 写一个简易 5×7 ASCII font
    extern void canvas_draw_text_5x7(int x, int y, const char *s, uint16_t color);
    canvas_draw_text_5x7(x, y, str, 0x0000);
}

void canvas_flush(void)
{
    // LVGL 会自动 dirty → flush, 这里 no-op
}

// ================ 简易 5x7 ASCII 字模（数字/字母） ================
//
//  取自经典 5×7 ASCII font (CHARLES M. WILKINS 简化版)
//
static const uint8_t kFont5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    // 简化: 只列关键字符
};

void canvas_draw_text_5x7(int x, int y, const char *s, uint16_t color)
{
    (void)kFont5x7;  // 字体表待补, 先占位
    // 占位: 仍使用 LVGL label 叠加
    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl, s);
    lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_set_size(lbl, 160 - x, 12);
    // 不删, 让 App 关闭时统一清理 (此处简化)
}

// ================ 按键桥接 ================
char button_peek_char(void)
{
    key_event_t e;
    if (!keys_pop(&e)) return 0;
    if (e.long_press) return 0;  // 略长按, 留给 App 自处理
    switch (e.code) {
        case KEY_UP:    return 'U';
        case KEY_DOWN:  return 'D';
        case KEY_LEFT:  return 'L';
        case KEY_RIGHT: return 'R';
        case KEY_A:     return 'A';
        case KEY_B:     return 'B';
        default: return 0;
    }
}

void button_wait_ms(uint32_t ms)
{
    // 阻塞直到按键事件
    int64_t deadline = esp_timer_get_time() + (int64_t)ms * 1000;
    while (1) {
        key_event_t e;
        if (keys_pop(&e)) return;
        if (ms && esp_timer_get_time() > deadline) return;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}