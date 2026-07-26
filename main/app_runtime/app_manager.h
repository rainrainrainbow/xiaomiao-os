#pragma once
#include "esp_err.h"
#include "desktop/desktop.h"

/* App 管理 */
esp_err_t app_manager_init(void);
void      app_manager_scan(void);                /* 扫描 SD/apps/ 目录 */
int       app_manager_get_count(void);
app_item_t *app_manager_get(int index);

/* 安装/卸载 */
esp_err_t app_manager_install(const char *app_path);   /* 从 .app(zip) 安装 */
esp_err_t app_manager_uninstall(const char *package);  /* 卸载 */

/* 运行 */
esp_err_t app_runtime_launch_app(app_item_t *app);
esp_err_t app_runtime_launch_system(const char *app_name);
void      app_runtime_stop(void);
bool      app_runtime_is_running(void);
