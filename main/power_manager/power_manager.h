/**
 * @file power_manager.h
 * @brief 电源管理模块 - 电池监测与自动休眠
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 电池状态
typedef enum {
    BATTERY_STATUS_UNKNOWN = 0,
    BATTERY_STATUS_CHARGING,
    BATTERY_STATUS_DISCHARGING,
    BATTERY_STATUS_FULL
} battery_status_t;

// 电源事件
typedef enum {
    POWER_EVENT_BATTERY_LOW,
    POWER_EVENT_BATTERY_CRITICAL,
    POWER_EVENT_SLEEP_ENTER,
    POWER_EVENT_SLEEP_EXIT,
    POWER_EVENT_WAKEUP
} power_event_t;

// 电源事件回调
typedef void (*power_event_callback_t)(power_event_t event);

/**
 * @brief 初始化电源管理器
 * @return ESP_OK 成功
 */
esp_err_t power_manager_init(void);

/**
 * @brief 启动电池监测任务
 * @return ESP_OK 成功
 */
esp_err_t power_manager_start_monitor(void);

/**
 * @brief 停止电池监测任务
 * @return ESP_OK 成功
 */
esp_err_t power_manager_stop_monitor(void);

/**
 * @brief 获取电池电量百分比
 * @return 电量百分比 (0-100)
 */
uint8_t power_manager_get_battery_level(void);

/**
 * @brief 获取电池电压
 * @return 电池电压 (mV)
 */
uint32_t power_manager_get_battery_voltage(void);

/**
 * @brief 获取电池状态
 * @return 电池状态
 */
battery_status_t power_manager_get_battery_status(void);

/**
 * @brief 设置低电量阈值
 * @param threshold 阈值百分比 (0-100)
 */
void power_manager_set_low_battery_threshold(uint8_t threshold);

/**
 * @brief 设置自动休眠时间
 * @param seconds 秒数，0表示禁用自动休眠
 */
void power_manager_set_auto_sleep_timeout(uint32_t seconds);

/**
 * @brief 进入休眠模式
 * @return ESP_OK 成功
 */
esp_err_t power_manager_enter_sleep(void);

/**
 * @brief 退出休眠模式
 * @return ESP_OK 成功
 */
esp_err_t power_manager_exit_sleep(void);

/**
 * @brief 检查是否处于休眠状态
 * @return true 休眠中
 */
bool power_manager_is_sleeping(void);

/**
 * @brief 重置休眠计时器（用户活动时调用）
 */
void power_manager_reset_sleep_timer(void);

/**
 * @brief 注册电源事件回调
 * @param callback 回调函数
 */
void power_manager_register_callback(power_event_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_H */
