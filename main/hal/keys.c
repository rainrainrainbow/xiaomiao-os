// ================ keys.c - 5 键（除 A 外）+ 长按检测 ================
//
//  硬件修订 (v0.2):
//    A 键不接独立 GPIO, 复用 ADC1_CH6 电池分压电路
//    → 状态由 hal/battery.c 提供, 这里只读 battery_a_is_pressed()
//
//  按键表:
//    UP=2, DOWN=13, LEFT=27, RIGHT=35, B=12
//    A   = (电池 ADC 阈值检测, 在 battery.c)

#include "keys.h"
#include "battery.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

static const char *TAG = "keys";

typedef struct {
    key_code_t code;
    gpio_num_t gpio;
} key_map_t;

static const key_map_t kKeyMap[] = {
    {KEY_UP,    GPIO_NUM_2},
    {KEY_DOWN,  GPIO_NUM_13},
    {KEY_LEFT,  GPIO_NUM_27},
    {KEY_RIGHT, GPIO_NUM_35},   // 仅输入，无内部上拉
    {KEY_B,     GPIO_NUM_12},
};

#define KEY_GPIO_COUNT  (sizeof(kKeyMap)/sizeof(kKeyMap[0]))

static QueueHandle_t s_evt_queue = NULL;

typedef struct {
    bool     pressed;
    int64_t  pressed_at_us;
    bool     long_fired;
} key_state_t;

static key_state_t s_g_state[KEY_GPIO_COUNT] = { 0 };
static key_state_t s_a_state = { 0 };

// 扫描任务: 6 键 (5 GPIO + 1 ADC)
static void keys_task(void *arg)
{
    while (1) {
        int64_t now = esp_timer_get_time();

        // 1. GPIO 扫描
        for (size_t i = 0; i < KEY_GPIO_COUNT; i++) {
            const key_map_t *km = &kKeyMap[i];
            key_state_t *st = &s_g_state[i];
            bool is_down = (gpio_get_level(km->gpio) == 0);

            if (is_down && !st->pressed) {
                st->pressed = true;
                st->pressed_at_us = now;
                st->long_fired = false;
                key_event_t e = { km->code, false };
                xQueueSend(s_evt_queue, &e, 0);
            } else if (!is_down && st->pressed) {
                st->pressed = false;
                st->long_fired = false;
            } else if (is_down && st->pressed && !st->long_fired) {
                if ((now - st->pressed_at_us) >= (KEY_LONG_PRESS_MS * 1000LL)) {
                    st->long_fired = true;
                    key_event_t e = { km->code, true };
                    xQueueSend(s_evt_queue, &e, 0);
                }
            }
        }

        // 2. A 键 ADC 扫描
        battery_a_update();
        bool a_down = battery_a_is_pressed();
        if (a_down && !s_a_state.pressed) {
            s_a_state.pressed = true;
            s_a_state.pressed_at_us = now;
            s_a_state.long_fired = false;
            key_event_t e = { KEY_A, false };
            xQueueSend(s_evt_queue, &e, 0);
        } else if (!a_down && s_a_state.pressed) {
            s_a_state.pressed = false;
            s_a_state.long_fired = false;
        } else if (a_down && s_a_state.pressed && !s_a_state.long_fired) {
            if ((now - s_a_state.pressed_at_us) >= (KEY_LONG_PRESS_MS * 1000LL)) {
                s_a_state.long_fired = true;
                key_event_t e = { KEY_A, true };
                xQueueSend(s_evt_queue, &e, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

esp_err_t keys_init(void)
{
    uint64_t mask = 0;
    for (size_t i = 0; i < KEY_GPIO_COUNT; i++) {
        mask |= (1ULL << kKeyMap[i].gpio);
    }

    gpio_config_t io = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    s_evt_queue = xQueueCreate(16, sizeof(key_event_t));
    if (!s_evt_queue) return ESP_ERR_NO_MEM;

    xTaskCreate(keys_task, "keys", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "keys init OK (5 GPIO + A via ADC)");
    return ESP_OK;
}

bool keys_pop(key_event_t *out)
{
    if (!s_evt_queue || !out) return false;
    return xQueueReceive(s_evt_queue, out, 0) == pdTRUE;
}

bool keys_is_pressed(key_code_t code)
{
    if (code == KEY_A) return s_a_state.pressed;
    for (size_t i = 0; i < KEY_GPIO_COUNT; i++) {
        if (kKeyMap[i].code == code) return s_g_state[i].pressed;
    }
    return false;
}