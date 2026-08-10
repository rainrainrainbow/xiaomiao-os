// ================ toast.h - 底部弹出提示 ================

#ifndef __TOAST_H__
#define __TOAST_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建 toast 容器（在最上层）
 */
void toast_init(lv_obj_t *parent);

/**
 * @brief 显示一条提示（默认 1.4s）
 *
 * 多次调用会重置计时器，新消息覆盖之前的
 */
void toast_show(const char *text, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // __TOAST_H__