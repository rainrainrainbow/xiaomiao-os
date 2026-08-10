// ================ mp_runtime.c - MicroPython 运行时（异步） ================
//
//  集成方式:
//    默认编译进 ESP-IDF Component Manager 的 micropython/micropython 组件
//    若 <py/runtime.h> 不可用, 自动降级到 STUB 模式 (编译仍能通过,
//    但所有 hal.* 调用无效, 桌面 UI 仍能正常显示).
//
//  执行方式:
//    mp_app_start()  → xTaskCreate(app_task)  → mp_parse_compile_execute()
//    App 的 while True 循环在独立任务里跑, 不会阻塞 LVGL.
//
//  沙盒:
//    每个 App 一个 mp_obj_dict_t globals, dict 里的 sys.app_id 由
//    mp_app_create 注入, hal.* 在 mp_bindings_register 时全量导入.
//
//  退出:
//    hal.sys_exit() 抛出 SystemExit, 任务自杀.
//    或外部 mp_app_stop() 设 stop_request 标志, 下次 hal.time_sleep_ms 时检测.

#include "mp_runtime.h"
#include "mp_bindings.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

// 真正启用 MicroPython 需要同时满足:
//   1. CONFIG_ENABLE_MICROPYTHON=y  (sdkconfig 里勾选)
//   2. idf_component.yml 加了 micropython/micropython 组件
// 否则只跑 STUB 模式 (App 仍是占位, 但桌面/UI 完整)
#if defined(CONFIG_ENABLE_MICROPYTHON) && __has_include(<py/runtime.h>)
#define MICROPYTHON_ENABLED 1
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/gc.h"
#include "py/objdict.h"
#include "py/mphal.h"
#else
#define MICROPYTHON_ENABLED 0
#endif

static const char *TAG = "mp";

static bool s_inited = false;

esp_err_t mp_runtime_init(void)
{
#if MICROPYTHON_ENABLED
    extern void mp_init(void);
    mp_init();
    mp_bindings_register();
    s_inited = true;
    ESP_LOGI(TAG, "MicroPython runtime init OK");
#else
    ESP_LOGW(TAG, "MicroPython disabled (stub mode) - enable via CONFIG_ENABLE_MICROPYTHON");
    s_inited = false;
#endif
    return ESP_OK;
}

void mp_runtime_deinit(void)
{
#if MICROPYTHON_ENABLED
    extern void mp_deinit(void);
    if (s_inited) mp_deinit();
#endif
    s_inited = false;
}

mp_app_ctx_t *mp_app_create(const char *id, const char *name, const char *version)
{
    mp_app_ctx_t *c = heap_caps_calloc(1, sizeof(mp_app_ctx_t), MALLOC_CAP_INTERNAL);
    if (!c) return NULL;
    strncpy(c->id, id, sizeof(c->id) - 1);
    strncpy(c->name, name, sizeof(c->name) - 1);
    strncpy(c->version, version, sizeof(c->version) - 1);
    c->permissions = MP_PERM_ALL;     // 默认全开, 调用方再覆盖
    c->running = false;
    c->stop_request = false;

#if MICROPYTHON_ENABLED
    mp_obj_t dict = mp_obj_new_dict(0);
    c->globals = MP_OBJ_TO_PTR(dict);
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(qstr_from_str("app_id")),
                      mp_obj_new_str(id, strlen(id)));
#else
    c->globals = NULL;
#endif

    ESP_LOGI(TAG, "app sandbox create: %s [%s v%s]", id, name, version);
    return c;
}

void mp_app_destroy(mp_app_ctx_t *ctx)
{
    if (!ctx) return;
    if (ctx->running) {
        mp_app_stop(ctx);
    }
#if MICROPYTHON_ENABLED
    // mpy GC 自动回收 globals 里的对象
#endif
    free(ctx);
    ESP_LOGI(TAG, "app sandbox freed");
}

// ================ 后台任务包装 ================
#if MICROPYTHON_ENABLED

typedef struct {
    mp_app_ctx_t *ctx;
    const char   *src;
} mp_task_arg_t;

static void mp_app_task(void *arg)
{
    mp_task_arg_t *ta = (mp_task_arg_t *)arg;
    mp_app_ctx_t *ctx = ta->ctx;
    const char *src = ta->src;
    free(ta);

    // 1. 装载源码到 ctx->source (mp_app_exec 用)
    strncpy(ctx->source, src, sizeof(ctx->source) - 1);

    // 2. 设置本 App 权限
    mp_bindings_set_active_perms(ctx->permissions);

    // 3. 执行 (允许 while True)
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_parse_compile_execute(MP_PARSE_SINGLE_INPUT,
                                 src,
                                 MP_OBJ_TO_PTR(ctx->globals));
        nlr_pop();
        ESP_LOGI(TAG, "[%s] python returned normally", ctx->id);
    } else {
        mp_obj_t exc = (mp_obj_t)nlr.ret_val;
        // SystemExit 是用户主动退出, 不算错误
        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(mp_obj_get_type(exc)),
                                    MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            ESP_LOGI(TAG, "[%s] sys_exit() called", ctx->id);
        } else {
            mp_obj_print_exception(&mp_plat_print, exc);
        }
    }

    ctx->running = false;
    ctx->task = NULL;
    vTaskDelete(NULL);   // 自杀
}

#endif  // MICROPYTHON_ENABLED

esp_err_t mp_app_start(mp_app_ctx_t *ctx, const char *py_source)
{
    if (!ctx || !py_source) return ESP_ERR_INVALID_ARG;
    if (ctx->running) {
        ESP_LOGW(TAG, "[%s] already running", ctx->id);
        return ESP_ERR_INVALID_STATE;
    }

#if MICROPYTHON_ENABLED
    mp_task_arg_t *ta = heap_caps_malloc(sizeof(mp_task_arg_t), MALLOC_CAP_INTERNAL);
    if (!ta) return ESP_ERR_NO_MEM;
    ta->ctx = ctx;
    ta->src = py_source;

    ctx->stop_request = false;
    ctx->running = true;

    BaseType_t r = xTaskCreatePinnedToCore(
        mp_app_task, "mp_app", 8192, ta, 4, &ctx->task, 0);
    if (r != pdPASS) {
        ctx->running = false;
        free(ta);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "[%s] task launched", ctx->id);
    return ESP_OK;
#else
    ESP_LOGW(TAG, "[%s] stub mode: skipping exec", ctx->id);
    (void)py_source;
    return ESP_OK;
#endif
}

void mp_app_stop(mp_app_ctx_t *ctx)
{
    if (!ctx) return;
    ctx->stop_request = true;
    // 真正实现里, 可以用 mp_sched_exception 让任务立刻跳出来.
    // 简化: 让任务在下一次 sleep 检测, 或直接 vTaskDelete.
#if MICROPYTHON_ENABLED
    if (ctx->task) {
        vTaskDelete(ctx->task);
        ctx->task = NULL;
        ctx->running = false;
        ESP_LOGW(TAG, "[%s] force killed", ctx->id);
    }
#endif
}

esp_err_t mp_app_exec(mp_app_ctx_t *ctx, const char *py_source)
{
    if (!ctx || !py_source) return ESP_ERR_INVALID_ARG;
#if MICROPYTHON_ENABLED
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_parse_compile_execute(MP_PARSE_SINGLE_INPUT,
                                 py_source,
                                 MP_OBJ_TO_PTR(ctx->globals));
        nlr_pop();
        return ESP_OK;
    } else {
        mp_obj_t exc = (mp_obj_t)nlr.ret_val;
        mp_obj_print_exception(&mp_plat_print, exc);
        return ESP_FAIL;
    }
#else
    (void)py_source;
    ESP_LOGW(TAG, "stub exec: %s", ctx->id);
    return ESP_OK;   // stub 永远成功
#endif
}