// ================ app_manager.h - App 管理与屏幕切换 ================

#ifndef __APP_MANAGER_H__
#define __APP_MANAGER_H__

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCREEN_HOME,
    SCREEN_APPS,
    SCREEN_SETTINGS,
    SCREEN_STORE,
    SCREEN_EDITOR,
    SCREEN_APP,        // 当前正在运行的 App
    SCREEN_MAX,
} screen_id_t;

typedef struct {
    const char *id;        // e.g. "com.demo.snake"
    const char *name;      // "贪吃蛇"
    const char *glyph;     // emoji
    const char *version;   // "1.0.2"
    bool  is_system;       // 系统 App（设置/商店/积木）
    bool  is_installed;    // 用户已安装
    const char *source_path;  // SD 卡 .app 文件路径（NULL 表示内置）
} app_info_t;

/**
 * @brief 初始化 App 注册表 + 状态机
 */
void app_manager_init(lv_obj_t *parent);

/**
 * @brief 切换屏幕
 */
void app_manager_show(screen_id_t id);

/**
 * @brief 启动一个 MicroPython App
 *        1. 创建沙盒 mp_app_ctx_t
 *        2. 设置当前 permissions
 *        3. 创建画布 canvas
 *        4. 执行 main.py
 *        5. 显示运行中屏（异步，可点 B 返回）
 */
void app_manager_launch(const app_info_t *app);

/**
 * @brief 强制结束当前运行 App（释放沙盒 + 杀后台任务）
 */
void app_manager_kill_running(void);

/**
 * @brief 当前屏
 */
screen_id_t app_manager_current(void);

/**
 * @brief 内置 App 列表（系统+Demo）
 *        用 app_info_t 数组暴露
 */
const app_info_t *app_manager_builtin_list(int *out_count);

/**
 * @brief 商店列表（可安装）
 */
const app_info_t *app_manager_store_list(int *out_count);

/**
 * @brief 安装/卸载
 */
void app_manager_install(const char *app_id);
void app_manager_uninstall(const char *app_id);
bool app_manager_is_installed(const char *app_id);

/**
 * @brief 持久化桌面排序到 NVS
 */
void app_manager_save_layout(void);
void app_manager_load_layout(void);

/**
 * @brief 桌面当前选中的 App 索引（供 arrangement mode 操作）
 */
int  app_manager_desktop_sel(void);
int  app_manager_desktop_page(void);
int  app_manager_desktop_total(void);
void app_manager_desktop_move_sel(int delta_col, int delta_row);
void app_manager_desktop_swap_on_page(int target);
void app_manager_desktop_set_move_mode(bool on);
bool app_manager_desktop_is_move_mode(void);

/**
 * @brief 当前屏事件分发（UI 层调用 main.c 时用）
 */
void app_manager_handle_a_press(void);
void app_manager_handle_b_press(void);
void app_manager_handle_long_a_press(void);
void app_manager_editor_num(int n);
void app_manager_editor_select_pane(int pane);
void app_manager_editor_sel(int delta);
void app_manager_editor_delete(void);
void app_manager_editor_insert(void);
void app_manager_list_move(int dir);

#ifdef __cplusplus
}
#endif

#endif // __APP_MANAGER_H__