/*
 * micropython_runtime.h — MicroPython VM integration for XiaoMiao OS
 *
 * Supports multiple concurrent apps (up to MAX_CONCURRENT_APPS).
 * Each app runs in its own VM instance with isolated heap.
 */
#pragma once
#include <stdbool.h>
#include "app_manager.h"

/* Maximum number of concurrent apps */
#define MAX_CONCURRENT_APPS 4

/* Initialize MicroPython runtime (call once at boot) */
bool micropython_init(void);

/* Launch an app: extract .app, initialize VM, execute main.py */
bool micropython_launch_app(const app_entry_t *app);

/* Stop a specific app by package name */
void micropython_stop_app(const char *package_name);

/* Stop all running apps */
void micropython_stop_all_apps(void);

/* Check if a specific app is running */
bool micropython_is_app_running(const char *package_name);

/* Check if any app is running */
bool micropython_is_running(void);

/* Get number of running apps */
int micropython_get_running_count(void);

/* Get running app package name by index (0..count-1), or NULL */
const char *micropython_get_running_app(int index);

/* Callback: app exited */
typedef void (*micropython_exit_cb_t)(const char *package_name, int exit_code);
void micropython_set_exit_callback(micropython_exit_cb_t cb);