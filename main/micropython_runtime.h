/*
 * micropython_runtime.h — MicroPython VM integration
 *
 * This module provides the interface for launching .app packages
 * that contain MicroPython scripts (main.py).
 *
 * Architecture:
 *   - MicroPython is integrated as an ESP-IDF component
 *   - Each .app is extracted to /tmp/<package>/
 *   - The VM is initialized with the app's directory as sys.path
 *   - main.py is executed in an isolated context
 *   - On exit, the VM is cleaned up and resources freed
 */
#pragma once
#include <stdbool.h>
#include "app_manager.h"

/* Initialize MicroPython runtime (call once at boot) */
bool micropython_init(void);

/* Launch an app: extract .app, initialize VM, execute main.py */
bool micropython_launch_app(const app_entry_t *app);

/* Stop the currently running app and cleanup */
void micropython_stop_app(void);

/* Check if an app is currently running */
bool micropython_is_running(void);

/* Get the package name of the currently running app (or NULL) */
const char *micropython_get_running_app(void);

/* Callback: app exited (called by MicroPython when main.py returns) */
typedef void (*micropython_exit_cb_t)(const char *package_name, int exit_code);
void micropython_set_exit_callback(micropython_exit_cb_t cb);