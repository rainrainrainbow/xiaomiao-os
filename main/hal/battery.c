// ================ battery.c - 电池电压 + A 键复用检测 ================

#include "battery.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

static const char *TAG = "battery";

static adc_oneshot_unit_handle_t s_adc = NULL;
static adc_cali_handle_t s_cali = NULL;
static bool s_calibrated = false;

// A 键状态机 (迟滞阈值避免抖动)
typedef enum { A_UNKNOWN, A_UP, A_DOWN } a_state_t;
static volatile a_state_t s_a_state = A_UP;   // 默认松开

esp_err_t battery_init(void)
{
    adc_oneshot_unit_init_cfg_t ucfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&ucfg, &s_adc), TAG, "adc unit");

    adc_oneshot_chan_cfg_t ccfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, BATTERY_ADC_CHANNEL, &ccfg),
                        TAG, "ch");

    // 工厂校准
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t curv = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_curve_fitting(&curv, &s_cali) == ESP_OK) {
        s_calibrated = true;
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t line = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    if (adc_cali_create_scheme_line_fitting(&line, &s_cali) == ESP_OK) {
        s_calibrated = true;
    }
#endif

    ESP_LOGI(TAG, "battery init OK (cal=%d, A-key shared on GPIO%d/ADC1_CH6)",
             s_calibrated, BATTERY_GPIO_NUM);
    return ESP_OK;
}

/**
 * @brief 读一次 ADC → 电压 V（ADC pin 处测得）
 */
static float read_adc_voltage(void)
{
    if (!s_adc) return 0;
    int raw = 0;
    adc_oneshot_read(s_adc, BATTERY_ADC_CHANNEL, &raw);

    int mv = 0;
    if (s_calibrated) {
        adc_cali_raw_to_voltage(s_cali, raw, &mv);
    } else {
        mv = (raw * 3300) / 4095;
    }
    return mv / 1000.0f;
}

/**
 * @brief A 键按下时 ADC ≈ 0V，未按时分压正常
 *        用双阈值迟滞
 */
void battery_a_update(void)
{
    float v = read_adc_voltage();

    switch (s_a_state) {
        case A_UP:
            if (v < A_KEY_DOWN_THRESHOLD) s_a_state = A_DOWN;
            break;
        case A_DOWN:
            if (v > A_KEY_UP_THRESHOLD) s_a_state = A_UP;
            break;
        default:
            s_a_state = (v < A_KEY_DOWN_THRESHOLD) ? A_DOWN : A_UP;
            break;
    }
}

bool battery_a_is_pressed(void) { return s_a_state == A_DOWN; }
float battery_a_adc_v(void)    { return read_adc_voltage(); }

float battery_voltage(void)
{
    if (s_a_state == A_DOWN) return 0.0f;  // A 按下时短路, ADC 无效
    float v_adc = read_adc_voltage();
    return v_adc * BATTERY_DIVIDER_RATIO;
}

float battery_level(void)
{
    float v = battery_voltage();
    if (v <= 3.0f) return 0.0f;
    if (v >= 4.2f) return 1.0f;
    return (v - 3.0f) / 1.2f;
}