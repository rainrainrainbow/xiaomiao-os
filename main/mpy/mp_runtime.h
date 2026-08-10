// ================ mp_runtime.h - MicroPython 运行时（异步后台任务版） ================
//
//  MicroPython 通过 ESP-IDF Component Manager 集成:
//    idf_component.yml:  micropython/micropython ^1.23
//
//  本模块职责:
//    1. 初始化 mpy 引擎 (mp_init)
//    2. 注册内置 C 模块 `hal` (mp_bindings)
//    3. 给每个 App 创建独立 globals() (沙盒)
//    4. App 启动 = 在单独 FreeRTOS 任务里执行 mp_parse_compile_execute
//      这样不会阻塞 LVGL 主任务
//    5. App 关闭时 kill 任务, free dict
//
//  内存预算:
//    mpy 内核 ≈ 200KB Flash, 16KB 静态 RAM
//    每个 App 运行时 dict ≈ 1KB, 任务栈 8KB (PSRAM)

#ifndef __MP_RUNTIME_H__
#define __MP_RUNTIME_H__

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mp_bindings.h"   // MP_PERM_* 定义来自这里, 保持单一来源

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明, 避免每次都 include mpy 头
struct _mp_obj_dict_t;

typedef struct {
    char     id[40];
    char     name[24];
    char     version[16];
    char     source[2048];       // .py 脚本源码
    struct _mp_obj_dict_t *globals;  // 沙盒
    uint8_t  permissions;        // 位掩码 (MP_PERM_*)

    // 后台任务控制
    TaskHandle_t task;
    volatile bool running;
    volatile bool stop_request;
} mp_app_ctx_t;

esp_err_t mp_runtime_init(void);
void       mp_runtime_deinit(void);

/** 创建 App 沙盒（不执行） */
mp_app_ctx_t *mp_app_create(const char *id, const char *name, const char *version);
void          mp_app_destroy(mp_app_ctx_t *ctx);

/**
 * 在后台 FreeRTOS 任务中异步执行 .py 源码
 *  - App 可以有 while True 循环, 不会阻塞 LVGL
 *  - 通过 mp_app_stop() 让 App 优雅退出
 *  - 返回 ESP_OK 表示任务已启动
 */
esp_err_t mp_app_start(mp_app_ctx_t *ctx, const char *py_source);

/** 请求停止 App（实际是请求任务自杀） */
void      mp_app_stop(mp_app_ctx_t *ctx);

/** 同步执行（保留兼容：仅用于一次性脚本片段，不要在 while True 中用） */
esp_err_t mp_app_exec(mp_app_ctx_t *ctx, const char *py_source);

#ifdef __cplusplus
}
#endif

#endif // __MP_RUNTIME_H__