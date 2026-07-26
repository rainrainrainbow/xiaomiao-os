/**
 * @file micropython_runtime.h
 * @brief MicroPython运行时集成
 */

#ifndef MICROPYTHON_RUNTIME_H
#define MICROPYTHON_RUNTIME_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化MicroPython运行时
 * @return ESP_OK 成功
 */
esp_err_t micropython_runtime_init(void);

/**
 * @brief 执行Python脚本
 * @param script_path 脚本路径
 * @return ESP_OK 成功
 */
esp_err_t micropython_execute_script(const char *script_path);

/**
 * @brief 停止当前运行的脚本
 * @return ESP_OK 成功
 */
esp_err_t micropython_stop_execution(void);

/**
 * @brief 检查是否有脚本正在运行
 * @return true 正在运行
 */
bool micropython_is_running(void);

/**
 * @brief 获取当前运行的脚本路径
 * @return 脚本路径，NULL表示无脚本运行
 */
const char *micropython_get_current_script(void);

/**
 * @brief 注册硬件API到MicroPython
 * @return ESP_OK 成功
 */
esp_err_t micropython_register_hardware_api(void);

#ifdef __cplusplus
}
#endif

#endif /* MICROPYTHON_RUNTIME_H */