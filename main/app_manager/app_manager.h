/**
 * @file app_manager.h
 * @brief 应用管理器 - 扫描SD卡、解析.app文件、管理应用列表
 */

#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 应用信息结构
typedef struct {
    char id[64];           // 应用ID
    char name[64];         // 应用名称
    char version[16];      // 版本号
    char author[64];       // 作者
    char description[128]; // 描述
    char icon_path[128];   // 图标路径
    char main_script[128]; // 主脚本路径
    bool is_system;        // 是否系统应用
    uint32_t size;         // 应用大小（字节）
} app_info_t;

// 应用列表
typedef struct {
    app_info_t *apps;      // 应用数组
    uint32_t count;        // 应用数量
    uint32_t capacity;     // 数组容量
} app_list_t;

/**
 * @brief 初始化应用管理器
 * @return ESP_OK 成功
 */
esp_err_t app_manager_init(void);

/**
 * @brief 扫描SD卡上的应用（结果存入全局列表）
 * @return ESP_OK 成功
 */
esp_err_t app_manager_scan_apps(void);

/**
 * @brief 解析.app文件
 * @param app_path .app文件路径
 * @param info 应用信息输出
 * @return ESP_OK 成功
 */
esp_err_t app_manager_parse_app(const char *app_path, app_info_t *info);

/**
 * @brief 获取应用列表
 * @return 应用列表指针
 */
app_list_t *app_manager_get_list(void);

/**
 * @brief 启动应用
 * @param app_id 应用ID
 * @return ESP_OK 成功
 */
esp_err_t app_manager_launch_app(const char *app_id);

/**
 * @brief 停止应用
 * @param app_id 应用ID
 * @return ESP_OK 成功
 */
esp_err_t app_manager_stop_app(const char *app_id);

/**
 * @brief 获取当前运行的应用
 * @return 应用ID，NULL表示无应用运行
 */
const char *app_manager_get_current_app(void);

/**
 * @brief 释放应用列表
 * @param list 应用列表指针
 */
void app_manager_free_list(app_list_t *list);

#ifdef __cplusplus
}
#endif

#endif /* APP_MANAGER_H */