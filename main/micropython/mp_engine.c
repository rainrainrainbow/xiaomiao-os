/*
 * mp_engine.c - MicroPython 引擎 (更新版)
 *
 * 使用 micropython/port/ 下的移植适配层
 */
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "micropython/mp_engine.h"
#include "micropython/mp_module_xiaomiao.h"

static const char *TAG = "mp_engine";

static bool      s_running = false;
static char      s_last_error[256] = {0};
static uint32_t  s_timeout_ms = 30000;

/* ---------- 初始化 ---------- */
esp_err_t mp_engine_init(void)
{
    ESP_LOGI(TAG, "Initializing MicroPython engine...");

    /* 调用移植层初始化 */
    extern void mp_init(void);
    mp_init();

    /* 注册 xiaomiao 系统模块 */
    mp_module_xiaomiao_register();

    ESP_LOGI(TAG, "  Heap free: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "  PSRAM free: %d bytes", esp_get_free_heap_size_psram());
    ESP_LOGI(TAG, "MicroPython engine ready");

    return ESP_OK;
}

/* ---------- 执行代码字符串 ---------- */
int mp_engine_exec(const char *code)
{
    if (!code || !*code) return -1;

    ESP_LOGI(TAG, "Executing %d bytes of Python code", (int)strlen(code));

    /* 日志前 200 字节 */
    char preview[201];
    strncpy(preview, code, 200);
    preview[200] = '\0';
    ESP_LOGD(TAG, "Code:\n%s", preview);

    s_running = true;

    /* 调用移植层 */
    extern int mp_exec_str(const char *code);
    int ret = mp_exec_str(code);

    s_running = false;

    if (ret != 0) {
        snprintf(s_last_error, sizeof(s_last_error),
                 "Execution failed (rc=%d)", ret);
        ESP_LOGE(TAG, "%s", s_last_error);
    }

    return ret;
}

int mp_engine_exec_file(const char *path)
{
    ESP_LOGI(TAG, "Executing file: %s", path);

    extern int mp_exec_file(const char *path);
    return mp_exec_file(path);
}

int mp_engine_exec_with_timeout(const char *code, uint32_t timeout_ms)
{
    s_timeout_ms = timeout_ms;
    return mp_engine_exec(code);
}

void mp_engine_stop(void)
{
    s_running = false;
    ESP_LOGI(TAG, "MicroPython execution stopped");
}

bool mp_engine_is_running(void) { return s_running; }

const char *mp_engine_get_error(void) { return s_last_error; }
