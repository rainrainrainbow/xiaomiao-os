/**
 * @file power_manager.c
 * @brief 电源管理模块实现 - 电池监测与自动休眠
 */

#include "power_manager.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <math.h>

static const char *TAG = "power_manager";

// 硬件配置
#define BATT_ADC_CHANNEL    ADC_CHANNEL_6
#define BATT_DIVIDER_RATIO  ((9.1 + 2.4) / 2.4)  // 分压比 4.79
#define ADC_SAMPLE_COUNT    10  // 多次采样取平均

// 电池电压阈值 (mV)
#define BATTERY_VOLTAGE_FULL        4200
#define BATTERY_VOLTAGE_NOMINAL     3700
#define BATTERY_VOLTAGE_EMPTY       3300
#define BATTERY_VOLTAGE_CRITICAL    3100

// 默认配置
#define DEFAULT_LOW_BATTERY_THRESHOLD   20  // 20%
#define DEFAULT_AUTO_SLEEP_TIMEOUT      300 // 5分钟

// 电源管理器状态
static struct {
    bool initialized;
    bool monitoring;
    TaskHandle_t monitor_task_handle;
    
    // ADC 句柄
    adc_oneshot_unit_handle_t adc1_handle;
    adc_cali_handle_t cali_handle;
    bool cali_enabled;
    
    // 电池状态
    uint32_t battery_voltage;  // mV
    uint8_t battery_level;     // 0-100%
    battery_status_t status;
    
    // 配置
    uint8_t low_battery_threshold;
    uint32_t auto_sleep_timeout;  // 秒
    
    // 休眠管理
    bool sleeping;
    uint32_t last_activity_time;  // 毫秒时间戳
    
    // 回调
    power_event_callback_t event_callback;
} s_pm = {0};

// 获取当前时间戳 (毫秒)
static uint32_t get_timestamp_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// ADC 采样并计算电压
static uint32_t read_battery_voltage(void)
{
    if (!s_pm.adc1_handle) {
        return 0;
    }
    
    int adc_raw_sum = 0;
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
        int raw;
        if (adc_oneshot_read(s_pm.adc1_handle, BATT_ADC_CHANNEL, &raw) == ESP_OK) {
            adc_raw_sum += raw;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    int adc_raw_avg = adc_raw_sum / ADC_SAMPLE_COUNT;
    
    // 转换为电压
    int voltage_mv = 0;
    if (s_pm.cali_enabled && s_pm.cali_handle) {
        adc_cali_raw_to_voltage(s_pm.cali_handle, adc_raw_avg, &voltage_mv);
    } else {
        // 无校准时使用近似公式
        // ESP32 ADC: 12位, 11dB衰减, 参考电压约1100mV
        voltage_mv = (adc_raw_avg * 1100 * 2) / 4095;
    }
    
    // 考虑分压电阻
    voltage_mv = (uint32_t)(voltage_mv * BATT_DIVIDER_RATIO);
    
    return voltage_mv;
}

// 根据电压计算电量百分比
static uint8_t voltage_to_percentage(uint32_t voltage_mv)
{
    if (voltage_mv >= BATTERY_VOLTAGE_FULL) {
        return 100;
    }
    if (voltage_mv <= BATTERY_VOLTAGE_CRITICAL) {
        return 0;
    }
    
    // 使用分段线性插值
    // 3.3V - 3.7V: 0% - 20%
    // 3.7V - 4.0V: 20% - 60%
    // 4.0V - 4.2V: 60% - 100%
    
    if (voltage_mv < 3700) {
        // 3.3V - 3.7V 映射到 0% - 20%
        float ratio = (float)(voltage_mv - 3300) / (3700 - 3300);
        return (uint8_t)(ratio * 20);
    } else if (voltage_mv < 4000) {
        // 3.7V - 4.0V 映射到 20% - 60%
        float ratio = (float)(voltage_mv - 3700) / (4000 - 3700);
        return (uint8_t)(20 + ratio * 40);
    } else {
        // 4.0V - 4.2V 映射到 60% - 100%
        float ratio = (float)(voltage_mv - 4000) / (4200 - 4000);
        return (uint8_t)(60 + ratio * 40);
    }
}

// 触发电源事件
static void trigger_event(power_event_t event)
{
    if (s_pm.event_callback) {
        s_pm.event_callback(event);
    }
}

// 更新电池状态
static void update_battery_status(void)
{
    // 读取电压
    s_pm.battery_voltage = read_battery_voltage();
    
    // 计算电量
    s_pm.battery_level = voltage_to_percentage(s_pm.battery_voltage);
    
    // 判断状态
    if (s_pm.battery_voltage >= BATTERY_VOLTAGE_FULL) {
        s_pm.status = BATTERY_STATUS_FULL;
    } else if (s_pm.battery_voltage > BATTERY_VOLTAGE_NOMINAL) {
        s_pm.status = BATTERY_STATUS_DISCHARGING;
    } else if (s_pm.battery_voltage < BATTERY_VOLTAGE_CRITICAL) {
        s_pm.status = BATTERY_STATUS_UNKNOWN;
        trigger_event(POWER_EVENT_BATTERY_CRITICAL);
    } else {
        s_pm.status = BATTERY_STATUS_DISCHARGING;
    }
    
    // 检查低电量
    if (s_pm.battery_level <= s_pm.low_battery_threshold) {
        trigger_event(POWER_EVENT_BATTERY_LOW);
    }
    
    ESP_LOGD(TAG, "Battery: %umV, %u%%", s_pm.battery_voltage, s_pm.battery_level);
}

// 检查自动休眠
static void check_auto_sleep(void)
{
    if (s_pm.auto_sleep_timeout == 0) {
        return;  // 禁用自动休眠
    }
    
    if (s_pm.sleeping) {
        return;  // 已经休眠
    }
    
    uint32_t current_time = get_timestamp_ms();
    uint32_t idle_time = (current_time - s_pm.last_activity_time) / 1000;  // 秒
    
    if (idle_time >= s_pm.auto_sleep_timeout) {
        ESP_LOGI(TAG, "Auto sleep triggered after %u seconds idle", idle_time);
        power_manager_enter_sleep();
    }
}

// 电池监测任务
static void battery_monitor_task(void *arg)
{
    ESP_LOGI(TAG, "Battery monitor task started");
    
    while (s_pm.monitoring) {
        // 更新电池状态
        update_battery_status();
        
        // 检查自动休眠
        check_auto_sleep();
        
        // 每5秒检测一次
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
    ESP_LOGI(TAG, "Battery monitor task stopped");
    s_pm.monitor_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t power_manager_init(void)
{
    if (s_pm.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing power manager");
    
    // 初始化 ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &s_pm.adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC: %s", esp_err_to_name(ret));
        return ret;
    }
    
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ret = adc_oneshot_config_channel(s_pm.adc1_handle, BATT_ADC_CHANNEL, &chan_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config ADC channel: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 初始化 ADC 校准
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = BATT_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &s_pm.cali_handle);
    if (ret == ESP_OK) {
        s_pm.cali_enabled = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    } else {
        s_pm.cali_enabled = false;
        ESP_LOGW(TAG, "ADC calibration not available, using approximate formula");
    }
    
    // 设置默认配置
    s_pm.low_battery_threshold = DEFAULT_LOW_BATTERY_THRESHOLD;
    s_pm.auto_sleep_timeout = DEFAULT_AUTO_SLEEP_TIMEOUT;
    s_pm.last_activity_time = get_timestamp_ms();
    
    // 初始读取电池状态
    update_battery_status();
    
    s_pm.initialized = true;
    ESP_LOGI(TAG, "Power manager initialized. Battery: %umV, %u%%", 
             s_pm.battery_voltage, s_pm.battery_level);
    
    return ESP_OK;
}

esp_err_t power_manager_start_monitor(void)
{
    if (!s_pm.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (s_pm.monitoring) {
        ESP_LOGW(TAG, "Already monitoring");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting battery monitor");
    
    s_pm.monitoring = true;
    BaseType_t ret = xTaskCreate(
        battery_monitor_task,
        "batt_monitor",
        4096,
        NULL,
        5,
        &s_pm.monitor_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create monitor task");
        s_pm.monitoring = false;
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t power_manager_stop_monitor(void)
{
    if (!s_pm.monitoring) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping battery monitor");
    
    s_pm.monitoring = false;
    
    // 等待任务结束
    if (s_pm.monitor_task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        s_pm.monitor_task_handle = NULL;
    }
    
    return ESP_OK;
}

uint8_t power_manager_get_battery_level(void)
{
    return s_pm.battery_level;
}

uint32_t power_manager_get_battery_voltage(void)
{
    return s_pm.battery_voltage;
}

battery_status_t power_manager_get_battery_status(void)
{
    return s_pm.status;
}

void power_manager_set_low_battery_threshold(uint8_t threshold)
{
    if (threshold > 100) {
        threshold = 100;
    }
    s_pm.low_battery_threshold = threshold;
    ESP_LOGI(TAG, "Low battery threshold set to %u%%", threshold);
}

void power_manager_set_auto_sleep_timeout(uint32_t seconds)
{
    s_pm.auto_sleep_timeout = seconds;
    ESP_LOGI(TAG, "Auto sleep timeout set to %u seconds", seconds);
}

esp_err_t power_manager_enter_sleep(void)
{
    if (s_pm.sleeping) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Entering sleep mode");
    
    s_pm.sleeping = true;
    trigger_event(POWER_EVENT_SLEEP_ENTER);
    
    // TODO: 实现实际的休眠逻辑
    // - 关闭 LCD 背光
    // - 降低 CPU 频率
    // - 进入 light sleep 或 deep sleep
    
    return ESP_OK;
}

esp_err_t power_manager_exit_sleep(void)
{
    if (!s_pm.sleeping) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Exiting sleep mode");
    
    s_pm.sleeping = false;
    s_pm.last_activity_time = get_timestamp_ms();
    
    trigger_event(POWER_EVENT_SLEEP_EXIT);
    trigger_event(POWER_EVENT_WAKEUP);
    
    // TODO: 实现实际的唤醒逻辑
    // - 开启 LCD 背光
    // - 恢复 CPU 频率
    
    return ESP_OK;
}

bool power_manager_is_sleeping(void)
{
    return s_pm.sleeping;
}

void power_manager_reset_sleep_timer(void)
{
    s_pm.last_activity_time = get_timestamp_ms();
    
    // 如果正在休眠，则唤醒
    if (s_pm.sleeping) {
        power_manager_exit_sleep();
    }
}

void power_manager_register_callback(power_event_callback_t callback)
{
    s_pm.event_callback = callback;
}