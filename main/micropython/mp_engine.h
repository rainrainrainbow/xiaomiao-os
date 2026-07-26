#pragma once
#include "esp_err.h"

/* MicroPython 引擎初始化 */
esp_err_t mp_engine_init(void);

/* 执行代码字符串 */
int  mp_engine_exec(const char *code);       /* 返回 0=成功 */
int  mp_engine_exec_file(const char *path);  /* 执行 .py 文件 */

/* 执行带超时保护的代码 */
int  mp_engine_exec_with_timeout(const char *code, uint32_t timeout_ms);

/* 停止当前执行 */
void mp_engine_stop(void);

/* 是否正在运行 */
bool mp_engine_is_running(void);

/* 获取上次错误 */
const char *mp_engine_get_error(void);
