#pragma once
#include "esp_err.h"
#include "desktop/desktop.h"

/* App 沙箱 - 在隔离环境中运行 MicroPython App
 *
 * 每个 App 拥有:
 *   - 独立的 /data/<package>/ 目录
 *   - 受限的系统 API 访问
 *   - 资源使用限制 (CPU/内存/时间)
 *   - 标准输出捕获
 */

typedef struct {
    char     package[64];
    uint32_t memory_limit;   /* 内存限制 (bytes) */
    uint32_t timeout_ms;     /* 执行超时 */
    bool     network_allowed;
    bool     storage_allowed;
} sandbox_config_t;

/* 沙箱管理 */
esp_err_t sandbox_create(const char *package, sandbox_config_t *cfg);
void      sandbox_destroy(const char *package);

/* 在沙箱中执行 */
esp_err_t sandbox_exec(const char *package, const char *script_path);
esp_err_t sandbox_exec_code(const char *package, const char *code);

/* 沙箱状态 */
bool      sandbox_is_running(const char *package);
uint32_t  sandbox_get_memory_usage(const char *package);
const char *sandbox_get_output(const char *package);
