/**
 * @file lvgl_port.c
 * @brief LVGL 移植层实现 - 显示驱动、按键输入和组合键检测
 */

#include "lvgl_port.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "lvgl_port";

// 显示参数
#define LCD_H_RES           160
#define LCD_V_RES           128
#define LVGL_TICK_PERIOD_MS 2

// LCD 句柄
static esp_lcd_panel_io_handle_t s_lcd_io = NULL;
static esp_lcd_panel_handle_t s_lcd_panel = NULL;

// LVGL 显示和输入设备
static lv_display_t *s_disp = NULL;
static lv_indev_t *s_indev = NULL;

// LVGL 互斥锁
static SemaphoreHandle_t s_lvgl_mutex = NULL;

// 按键 GPIO 定义
#define BTN_PIN_UP      GPIO_NUM_2
#define BTN_PIN_DOWN    GPIO_NUM_13
#define BTN_PIN_LEFT    GPIO_NUM_27
#define BTN_PIN_RIGHT   GPIO_NUM_35
#define BTN_PIN_A       GPIO_NUM_34
#define BTN_PIN_B       GPIO_NUM_12

// 按键去抖时间
#define DEBOUNCE_MS     20

// 组合键超时（毫秒）
#define COMBO_TIMEOUT_MS 500

// 按键状态
typedef struct {
    button_id_t id;
    gpio_num_t gpio;
    bool pressed;
    bool last_state;
    uint32_t last_change_ms;
} button_state_t;

static button_state_t s_buttons[BTN_COUNT] = {
    {BTN_UP,    BTN_PIN_UP,    false, false, 0},
    {BTN_DOWN,  BTN_PIN_DOWN,  false, false, 0},
    {BTN_LEFT,  BTN_PIN_LEFT,  false, false, 0},
    {BTN_RIGHT, BTN_PIN_RIGHT, false, false, 0},
    {BTN_A,     BTN_PIN_A,     false, false, 0},
    {BTN_B,     BTN_PIN_B,     false, false, 0},
};

// 组合键回调
typedef struct {
    button_id_t key1;
    button_id_t key2;
    combo_key_callback_t callback;
} combo_key_t;

#define MAX_COMBO_KEYS 4
static combo_key_t s_combo_keys[MAX_COMBO_KEYS];
static int s_combo_count = 0;

// 组合键检测状态
static button_id_t s_last_pressed_key = BTN_COUNT;
static uint32_t s_last_pressed_time = 0;

// 单键事件回调
static key_event_callback_t s_key_event_cb = NULL;

esp_err_t lvgl_port_input_init(void)
{
    ESP_LOGI(TAG, "Initializing button input");
    
    // 配置所有按键 GPIO
    for (int i = 0; i < BTN_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << s_buttons[i].gpio,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", 
                     s_buttons[i].gpio, esp_err_to_name(ret));
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "Button input initialized successfully");
    return ESP_OK;
}

void lvgl_port_register_combo_key(const char *combo, combo_key_callback_t callback)
{
    if (s_combo_count >= MAX_COMBO_KEYS) {
        ESP_LOGW(TAG, "Max combo keys reached");
        return;
    }
    
    combo_key_t *ck = &s_combo_keys[s_combo_count];
    ck->callback = callback;
    
    if (strcmp(combo, "up+b") == 0) {
        ck->key1 = BTN_UP;
        ck->key2 = BTN_B;
    } else if (strcmp(combo, "down+b") == 0) {
        ck->key1 = BTN_DOWN;
        ck->key2 = BTN_B;
    } else {
        ESP_LOGW(TAG, "Unknown combo: %s", combo);
        return;
    }
    
    s_combo_count++;
    ESP_LOGI(TAG, "Registered combo key: %s", combo);
}

void lvgl_port_register_key_event(key_event_callback_t callback)
{
    s_key_event_cb = callback;
    ESP_LOGI(TAG, "Registered key event callback");
}

void lvgl_port_keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    // 检查是否有按键按下
    for (int i = 0; i < BTN_COUNT; i++) {
        if (s_buttons[i].pressed) {
            data->state = LV_INDEV_STATE_PRESSED;
            
            // 映射到 LVGL 键值
            switch (s_buttons[i].id) {
                case BTN_UP:    data->key = LV_KEY_UP; break;
                case BTN_DOWN:  data->key = LV_KEY_DOWN; break;
                case BTN_LEFT:  data->key = LV_KEY_LEFT; break;
                case BTN_RIGHT: data->key = LV_KEY_RIGHT; break;
                case BTN_A:     data->key = LV_KEY_ENTER; break;
                case BTN_B:     data->key = LV_KEY_ESC; break;
                default:        data->key = 0; break;
            }
            return;
        }
    }
    
    data->state = LV_INDEV_STATE_RELEASED;
}

bool lvgl_port_is_button_pressed(button_id_t btn)
{
    if (btn >= BTN_COUNT) return false;
    return s_buttons[btn].pressed;
}

void lvgl_port_button_scan_task(void *arg)
{
    ESP_LOGI(TAG, "Button scan task started");
    
    while (1) {
        uint32_t now = lv_tick_get();
        
        // 扫描所有按键
        for (int i = 0; i < BTN_COUNT; i++) {
            button_state_t *btn = &s_buttons[i];
            bool current = (gpio_get_level(btn->gpio) == 0);  // 低电平有效
            
            // 去抖处理
            if (current != btn->last_state) {
                if (now - btn->last_change_ms >= DEBOUNCE_MS) {
                    btn->last_state = current;
                    btn->last_change_ms = now;
                    
                    if (current && !btn->pressed) {
                        // 按键按下
                        btn->pressed = true;
                        ESP_LOGD(TAG, "Button %d pressed", btn->id);
                        
                        // 组合键检测
                        if (s_last_pressed_key != BTN_COUNT && 
                            (now - s_last_pressed_time) < COMBO_TIMEOUT_MS) {
                            // 检查是否匹配组合键
                            for (int j = 0; j < s_combo_count; j++) {
                                combo_key_t *ck = &s_combo_keys[j];
                                if ((ck->key1 == s_last_pressed_key && ck->key2 == btn->id) ||
                                    (ck->key1 == btn->id && ck->key2 == s_last_pressed_key)) {
                                    ESP_LOGI(TAG, "Combo key detected: %d+%d", 
                                             s_last_pressed_key, btn->id);
                                    if (ck->callback) {
                                        ck->callback();
                                    }
                                    // 清除状态，避免重复触发
                                    btn->pressed = false;
                                    s_buttons[s_last_pressed_key].pressed = false;
                                    s_last_pressed_key = BTN_COUNT;
                                    break;
                                }
                            }
                        }
                        
                        // 记录最后按下的键
                        s_last_pressed_key = btn->id;
                        s_last_pressed_time = now;
                        
                        // 触发单键事件回调（延迟触发，等待组合键窗口）
                        // 实际在按键释放时触发
                    } else if (!current && btn->pressed) {
                        // 按键释放
                        btn->pressed = false;
                        ESP_LOGD(TAG, "Button %d released", btn->id);
                        
                        // 触发单键事件（如果不是组合键的一部分）
                        if (s_key_event_cb && s_last_pressed_key == btn->id) {
                            lv_key_t lv_key = 0;
                            switch (btn->id) {
                                case BTN_UP:    lv_key = LV_KEY_UP; break;
                                case BTN_DOWN:  lv_key = LV_KEY_DOWN; break;
                                case BTN_LEFT:  lv_key = LV_KEY_LEFT; break;
                                case BTN_RIGHT: lv_key = LV_KEY_RIGHT; break;
                                case BTN_A:     lv_key = LV_KEY_ENTER; break;
                                case BTN_B:     lv_key = LV_KEY_ESC; break;
                                default: break;
                            }
                            if (lv_key != 0) {
                                s_key_event_cb(lv_key);
                            }
                            s_last_pressed_key = BTN_COUNT;
                        }
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms 扫描周期
    }
}

/* ── LVGL 显示驱动 ─────────────────────────────────────────────────── */

// 显示刷新完成回调
static bool flush_ready_cb(esp_lcd_panel_io_handle_t panel_io, 
                           esp_lcd_panel_io_event_data_t *edata, 
                           void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

// 显示刷新回调
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    
    int32_t x1 = area->x1;
    int32_t x2 = area->x2;
    int32_t y1 = area->y1;
    int32_t y2 = area->y2;
    
    esp_lcd_panel_draw_bitmap(panel, x1, y1, x2 + 1, y2 + 1, px_map);
}

// LVGL tick 回调
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

esp_err_t lvgl_port_init(esp_lcd_panel_io_handle_t lcd_io, esp_lcd_panel_handle_t lcd_panel)
{
    ESP_LOGI(TAG, "Initializing LVGL port");
    
    s_lcd_io = lcd_io;
    s_lcd_panel = lcd_panel;
    
    // 创建互斥锁
    s_lvgl_mutex = xSemaphoreCreateMutex();
    if (s_lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return ESP_FAIL;
    }
    
    // 创建 LVGL 显示
    s_disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    if (s_disp == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_FAIL;
    }
    
    // 分配绘制缓冲区（使用 PSRAM）
    size_t buf_size = LCD_H_RES * LCD_V_RES * sizeof(lv_color_t);
    lv_color_t *buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    lv_color_t *buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    
    if (buf1 == NULL || buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers");
        return ESP_ERR_NO_MEM;
    }
    
    lv_display_set_buffers(s_disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(s_disp, flush_cb);
    lv_display_set_user_data(s_disp, s_lcd_panel);
    
    // 注册刷新完成回调
    esp_lcd_panel_io_register_event_callbacks(s_lcd_io, flush_ready_cb, s_disp);
    
    // 初始化按键输入
    esp_err_t ret = lvgl_port_input_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize input");
        return ret;
    }
    
    // 创建 LVGL 输入设备
    s_indev = lv_indev_create();
    if (s_indev == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL input device");
        return ESP_FAIL;
    }
    
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_indev, lvgl_port_keypad_read_cb);
    
    // 创建 LVGL tick 定时器
    esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    
    esp_timer_handle_t tick_timer = NULL;
    ret = esp_timer_create(&tick_timer_args, &tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LVGL tick timer");
        return ret;
    }
    
    ret = esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL tick timer");
        return ret;
    }
    
    ESP_LOGI(TAG, "LVGL port initialized successfully");
    return ESP_OK;
}

void lvgl_port_task_handler(void)
{
    if (s_lvgl_mutex == NULL) {
        return;
    }
    
    if (xSemaphoreTake(s_lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        lv_timer_handler();
        xSemaphoreGive(s_lvgl_mutex);
    }
}