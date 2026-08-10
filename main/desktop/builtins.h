// ================ builtins.h - 内置 App 注册 ================

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include "lvgl.h"
#include "core/app_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册 14 个内置 App
 *
 * 因为大部分是占位演示），只需要在启动屏上运行一个基本演示循环
 * 真实逻辑扩展点位于：
 *   app_xxx_run(ctx) - 在 app_view 屏上运行的回调
 */
void builtins_register(void);

/**
 * @brief 在 SCREEN_APP 屏上运行某个 App 的"演示"
 *        每帧调用一次，由 LVGL 定时器驱动
 */
void builtins_run_step(const app_info_t *app, uint32_t tick_ms);

#ifdef __cplusplus
}
#endif

#endif // __BUILTINS_H__