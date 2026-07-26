#pragma once
#include "lvgl.h"
#include "desktop/desktop.h"

/* App 启动器 - 处理 App 的安装/卸载/启动流程 */

void launcher_open(void);          /* 打开应用列表 */
void launcher_handle_key(int key);
void launcher_install_app(const char *path);  /* 安装 .app */
void launcher_uninstall_app(int index);        /* 卸载 */
