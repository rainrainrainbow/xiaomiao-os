/*
 * keys.c - 6 键扫描 + 消抖 + 长按检测
 *
 * 引脚: UP=2 DOWN=13 LEFT=27 RIGHT=35 A=34 B=12
 * 低电平有效。GPIO34/35 仅输入，无内部上拉。
 *
 * 电池检测复用 KEY_A 引脚 (ADC1_CH6, 分压 9.1k/2.4k)
 *   电池电压 = ADC_voltage * (9.1+2.4)/2.4
 */

#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal/keys.h"

static const char *TAG = "keys";

/* 消抖参数 */
#define DEBOUNCE_MS   20
#define LONG_PRESS_MS 500

/* 按键状态机 */
typedef struct {
    uint8_t  pin;
    bool     has_pullup;
    bool     last_raw;       /* 上次原始电平 */
    bool     stable_state;   /* 消抖后状态 (true=pressed) */
    uint32_t last_change_ms;
    bool     long_fired;
    uint32_t press_start_ms;
} key_state_t;

static key_state_t s_keys[KEY_COUNT] = {
    [KEY_UP]    = {.pin = KEY_UP_PIN,    .has_pullup = true},
    [KEY_DOWN]  = {.pin = KEY_DOWN_PIN,  .has_pullup = true},
    [KEY_LEFT]  = {.pin = KEY_LEFT_PIN,  .has_pullup = true},
    [KEY_RIGHT] = {.pin = KEY_RIGHT_PIN, .has_pullup = false}, /* GPIO35 */
    [KEY_A]     = {.pin = KEY_A_PIN,     .has_pullup = false}, /* GPIO34 */
    [KEY_B]     = {.pin = KEY_B_PIN,     .has_pullup = true},
};

static key_callback_t s_callback = NULL;

/* ADC 句柄 (用于电池检测) */
static adc_oneshot_unit_handle_t s_adc_unit = NULL;

/* ---------- 内部函数 ---------- */
static bool key_read_raw(key_code_t k)
{
    return (gpio_get_level(s_keys[k].pin) == 0); /* 低电平 = 按下 */
}

/* ---------- 公开 API ---------- */
esp_err_t keys_init(void)
{
    ESP_LOGI(TAG, "Initializing 6-key matrix...");

    for (int i = 0; i < KEY_COUNT; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = BIT64(s_keys[i].pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = s_keys[i].has_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        s_keys[i].last_raw = key_read_raw(i);
        s_keys[i].stable_state = s_keys[i].last_raw;
        s_keys[i].last_change_ms = 0;
    }

    /* ADC 初始化（电池检测，KEY_A 引脚 = GPIO34 = ADC1_CH6） */
    keys_battery_adc_init();

    ESP_LOGI(TAG, "Keys ready. UP=%d DOWN=%d LEFT=%d RIGHT=%d A=%d(ADC) B=%d",
             KEY_UP_PIN, KEY_DOWN_PIN, KEY_LEFT_PIN, KEY_RIGHT_PIN, KEY_A_PIN, KEY_B_PIN);
    return ESP_OK;
}

void keys_update(void)
{
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    for (int i = 0; i < KEY_COUNT; i++) {
        /* KEY_A 复用为电池检测时，跳过按键读取 */
        if (i == KEY_A && s_adc_unit != NULL) {
            /* 电池检测模式下 A 键不可用 */
            continue;
        }

        bool raw = key_read_raw(i);
        key_state_t *ks = &s_keys[i];

        if (raw != ks->last_raw) {
            ks->last_change_ms = now;
            ks->last_raw = raw;
        }

        if ((now - ks->last_change_ms) >= DEBOUNCE_MS) {
            if (raw != ks->stable_state) {
                ks->stable_state = raw;
                if (raw) {
                    /* 按下 */
                    ks->press_start_ms = now;
                    ks->long_fired = false;
                    if (s_callback) s_callback(i, KEY_EVENT_PRESS);
                } else {
                    /* 释放 */
                    if (s_callback) s_callback(i, KEY_EVENT_RELEASE);
                }
            }

            /* 长按检测 */
            if (raw && !ks->long_fired && (now - ks->press_start_ms) >= LONG_PRESS_MS) {
                ks->long_fired = true;
                if (s_callback) s_callback(i, KEY_EVENT_HOLD);
            }
        }
    }
}

key_code_t keys_scan(void)
{
    for (int i = 0; i < KEY_COUNT; i++) {
        if (i == KEY_A && s_adc_unit != NULL) continue;
        if (s_keys[i].stable_state) return (key_code_t)i;
    }
    return KEY_NONE;
}

bool key_is_pressed(key_code_t k)
{
    if (k == KEY_A && s_adc_unit != NULL) return false;
    if (k < 0 || k >= KEY_COUNT) return false;
    return s_keys[k].stable_state;
}

void keys_set_callback(key_callback_t cb)
{
    s_callback = cb;
}

/* ---------- 电池检测 (ADC) ---------- */
void keys_battery_adc_init(void)
{
    /* GPIO34 = ADC1_CH6 */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    if (adc_oneshot_new_unit(&init_cfg, &s_adc_unit) != ESP_OK) {
        ESP_LOGW(TAG, "ADC init failed - battery monitoring disabled");
        s_adc_unit = NULL;
        return;
    }

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = ADC_ATTEN_DB_11,     /* 0-3.3V 量程 */
        .bitwidth = ADC_BITWIDTH_12,  /* 12-bit */
    };
    adc_oneshot_config_channel(s_adc_unit, ADC_CHANNEL_6, &ch_cfg);
    ESP_LOGI(TAG, "Battery ADC initialized (GPIO34, 9.1k/2.4k divider)");
}

uint16_t keys_battery_read_raw(void)
{
    if (!s_adc_unit) return 0;
    int raw = 0;
    adc_oneshot_read(s_adc_unit, ADC_CHANNEL_6, &raw);
    return (uint16_t)raw;
}

float keys_battery_voltage(void)
{
    uint16_t raw = keys_battery_read_raw();
    if (raw == 0) return 0.0f;

    /* ADC 值 → 电压 (mV)  → 电池电压 (分压比 = (9.1+2.4)/2.4 = 4.792) */
    float adc_voltage = (raw / 4095.0f) * 3300.0f;  /* mV */
    float battery_mv = adc_voltage * (9.1f + 2.4f) / 2.4f;
    return battery_mv / 1000.0f;  /* V */
}
