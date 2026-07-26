/*
 * mp_port_xiaomiao.c - MicroPython 移植适配层
 *
 * 这是 MicroPython 在 XiaoMiao OS 上的精简移植。
 * 不依赖完整 MicroPython ESP32 port，而是提供最小运行时:
 *
 *   - mp_init() / mp_deinit()
 *   - mp_exec_str() - 执行 Python 代码字符串
 *   - mp_register_module() - 注册 C 模块
 *
 * 当完整 MicroPython 子模块可用时，此文件作为兼容层。
 * 当不可用时，提供降级实现（伪执行 + 日志）。
 *
 * 编译宏:
 *   WITH_MICROPYTHON - 启用真实 MP 解释器
 *   WITHOUT_MICROPYTHON - 降级模式（仅日志）
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "micropython/mp_engine.h"
#include "micropython/mp_module_xiaomiao.h"

static const char *TAG = "mp_port";

/* ========== 降级模式实现 ========== */
#ifndef WITH_MICROPYTHON

static bool s_mp_initialized = false;
static char s_mp_output[1024];
static int  s_mp_output_pos = 0;

void mp_init(void)
{
    ESP_LOGI(TAG, "MicroPython init (lite mode)");
    s_mp_initialized = true;
    s_mp_output_pos = 0;
}

void mp_deinit(void)
{
    s_mp_initialized = false;
    ESP_LOGI(TAG, "MicroPython deinit");
}

/* 降级: 解析并执行简单的 print/赋值语句 */
int mp_exec_str(const char *code)
{
    if (!s_mp_initialized) mp_init();

    ESP_LOGI(TAG, "[MP exec] %d bytes", (int)strlen(code));

    /* 检测 import 语句 */
    if (strstr(code, "import xiaomiao") || strstr(code, "from xiaomiao")) {
        ESP_LOGI(TAG, "[MP] xiaomiao module available");
    }

    /* 检测 print 语句 - 提取内容到输出 */
    const char *p = strstr(code, "print(");
    while (p) {
        p += 6; /* skip "print(" */
        const char *end = strchr(p, ')');
        if (end) {
            int len = end - p;
            if (len > 0 && len < 200) {
                char tmp[256];
                memcpy(tmp, p, len);
                tmp[len] = '\0';
                /* 去除引号 */
                if (tmp[0] == '"' || tmp[0] == '\''') {
                    memmove(tmp, tmp+1, strlen(tmp));
                    tmp[strlen(tmp)-1] = '\0';
                }
                ESP_LOGI(TAG, "[MP stdout] %s", tmp);
            }
        }
        p = strstr(end, "print(");
    }

    /* 检测 screen.text() 调用 */
    if (strstr(code, "screen.text")) {
        ESP_LOGI(TAG, "[MP] screen.text() called");
    }
    if (strstr(code, "music.tone")) {
        ESP_LOGI(TAG, "[MP] music.tone() called");
    }
    if (strstr(code, "time.sleep")) {
        ESP_LOGI(TAG, "[MP] time.sleep() called");
    }

    return 0; /* 总是成功（降级模式） */
}

int mp_exec_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open: %s", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    int ret = mp_exec_str(buf);
    free(buf);
    return ret;
}

void mp_register_module(const char *name, void *module_obj)
{
    ESP_LOGD(TAG, "Registered module: %s", name);
}

const char *mp_get_output(void) { return s_mp_output; }

/* ========== 真实 MicroPython 集成接口 ========== */
#else

/* 当定义了 WITH_MICROPYTHON 时，链接真实 MP 库 */
#include "py/mpstate.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/gc.h"

void mp_init(void)
{
    ESP_LOGI(TAG, "MicroPython init (full mode)");
    /* 初始化 MP 堆 */
    static uint8_t *heap_start = NULL;
    if (!heap_start) {
        heap_start = heap_caps_malloc(MICROPY_HEAP_SIZE, MALLOC_CAP_SPIRAM);
        if (!heap_start) {
            ESP_LOGE(TAG, "Failed to alloc MP heap in PSRAM");
            return;
        }
    }

    /* 设置 GC 堆 */
    gc_init(heap_start, heap_start + MICROPY_HEAP_SIZE);

    /* 初始化 MP 状态 */
    mp_init_globals();
    mp_init_state();

    /* 注册 xiaomiao 模块 */
    mp_module_xiaomiao_register();

    ESP_LOGI(TAG, "MicroPython ready. Heap: %d bytes in PSRAM", MICROPY_HEAP_SIZE);
}

void mp_deinit(void)
{
    ESP_LOGI(TAG, "MicroPython deinit");
    gc_sweep_all();
}

int mp_exec_str(const char *code)
{
    ESP_LOGI(TAG, "[MP exec] %d bytes", (int)strlen(code));

    /* 编译并执行 */
    mp_lexer_t *lex = mp_lexer_new_from_str_len(0, code, strlen(code), 0);
    if (!lex) {
        ESP_LOGE(TAG, "Lexer creation failed");
        return -1;
    }

    mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
    mp_lexer_free(lex);

    if (parse_tree.root == NULL) {
        ESP_LOGE(TAG, "Parse failed");
        return -1;
    }

    mp_obj_t module_fun = mp_compile(&parse_tree, "user_code", MP_EMIT_OPT_NONE, false);
    if (module_fun == MP_OBJ_NULL) {
        ESP_LOGE(TAG, "Compile failed");
        return -1;
    }

    mp_call_function_0(module_fun);
    return 0;
}

int mp_exec_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    int ret = mp_exec_str(buf);
    free(buf);
    return ret;
}

void mp_register_module(const char *name, void *module_obj)
{
    /* 注册到 MP 模块字典 */
    mp_store_global(mp_load_name(qstr_from_str(name)), (mp_obj_t)module_obj);
}

const char *mp_get_output(void) { return ""; }

#endif /* WITH_MICROPYTHON */
