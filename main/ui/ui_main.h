/**
 * @file ui_main.h
 * @brief 主UI界面 - 基于LVGL的桌面系统
 */

#ifndef UI_MAIN_H
#define UI_MAIN_H

#include "lvgl.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化UI系统
 * @return ESP_OK 成功
 */
esp_err_t ui_main_init(void);

/**
 * @brief 显示主屏幕
 */
void ui_show_home(void);

/**
 * @brief 显示应用列表
 */
void ui_show_app_list(void);

/**
 * @brief 显示设置界面
 */
void ui_show_settings(void);

/**
 * @brief 显示任务管理器
 */
void ui_show_task_manager(void);

/**
 * @brief 返回上一界面
 */
void ui_go_back(void);

/**
 * @brief 更新应用列表
 */
void ui_update_app_list(void);

/**
 * @brief 设置当前主题
 * @param theme_name 主题名称
 */
void ui_set_theme(const char *theme_name);

/**
 * @brief 处理按键输入
 * @param key LVGL键值
 */
void ui_handle_key(lv_key_t key);

/**
 * @brief 添加运行中的应用
 * @param id 应用ID
 * @param name 应用名称
 */
void ui_add_running_app(const char *id, const char *name);

/**
 * @brief 移除运行中的应用
 * @param id 应用ID
 */
void ui_remove_running_app(const char *id);

/**
 * @brief 切换应用锁定状态
 * @param id 应用ID
 */
void ui_toggle_app_lock(const char *id);

/**
 * @brief 检查应用是否锁定
 * @param id 应用ID
 * @return true 已锁定
 */
bool ui_is_app_locked(const char *id);

#ifdef __cplusplus
}
#endif

#endif /* UI_MAIN_H */