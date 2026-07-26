/**
 * @file config_manager.h
 * @brief 配置管理器 - JSON配置持久化
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 配置键名
#define CONFIG_KEY_THEME        "theme"
#define CONFIG_KEY_BRIGHTNESS   "brightness"
#define CONFIG_KEY_VOLUME       "volume"
#define CONFIG_KEY_WIFI_SSID    "wifi_ssid"
#define CONFIG_KEY_WIFI_PASS    "wifi_pass"
#define CONFIG_KEY_AUTO_SLEEP   "auto_sleep"
#define CONFIG_KEY_LANGUAGE     "language"

// 配置文件路径
#define CONFIG_FILE_PATH        "/sdcard/.xiaomiao/config.json"

/**
 * @brief 初始化配置管理器
 * @return ESP_OK 成功
 */
esp_err_t config_manager_init(void);

/**
 * @brief 加载配置文件
 * @return ESP_OK 成功
 */
esp_err_t config_manager_load(void);

/**
 * @brief 保存配置文件
 * @return ESP_OK 成功
 */
esp_err_t config_manager_save(void);

/**
 * @brief 获取整数配置
 * @param key 配置键
 * @param default_value 默认值
 * @return 配置值
 */
int config_manager_get_int(const char *key, int default_value);

/**
 * @brief 设置整数配置
 * @param key 配置键
 * @param value 配置值
 */
void config_manager_set_int(const char *key, int value);

/**
 * @brief 获取字符串配置
 * @param key 配置键
 * @param default_value 默认值
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 */
void config_manager_get_str(const char *key, const char *default_value, 
                            char *buffer, size_t buffer_size);

/**
 * @brief 设置字符串配置
 * @param key 配置键
 * @param value 配置值
 */
void config_manager_set_str(const char *key, const char *value);

/**
 * @brief 获取布尔配置
 * @param key 配置键
 * @param default_value 默认值
 * @return 配置值
 */
bool config_manager_get_bool(const char *key, bool default_value);

/**
 * @brief 设置布尔配置
 * @param key 配置键
 * @param value 配置值
 */
void config_manager_set_bool(const char *key, bool value);

/**
 * @brief 删除配置项
 * @param key 配置键
 */
void config_manager_delete(const char *key);

/**
 * @brief 重置所有配置为默认值
 */
void config_manager_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_MANAGER_H */