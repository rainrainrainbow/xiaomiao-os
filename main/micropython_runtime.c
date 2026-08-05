/*
 * micropython_runtime.c — MicroPython VM integration
 *
 * Provides full VM lifecycle for .app packages:
 *   1. Extract .app (ZIP) to /tmp/<package>/
 *   2. Initialize MicroPython GC heap + sys.path
 *   3. Execute main.py
 *   4. Cleanup on exit
 */
#include "micropython_runtime.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/compile.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"
#include "xiaomiao_modules.h"
#include "miniz.h"

static const char *TAG = "MPY";

/* GC heap size: 256KB from PSRAM */
#define MPY_HEAP_SIZE (256 * 1024)

/* Runtime state */
static struct {
    bool initialized;
    bool running;
    char current_app[APP_ID_MAX_LEN];
    char extract_path[APP_PATH_MAX_LEN];
    micropython_exit_cb_t exit_cb;
    void *gc_heap;
} s_runtime = {0};

/* ── ZIP Extraction ─────────────────────────────────────────────────────── */

static bool extract_app_zip(const char *zip_path, const char *dest_dir)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, zip_path, 0)) {
        ESP_LOGE(TAG, "Failed to open ZIP: %s", zip_path);
        return false;
    }

    int file_count = (int)mz_zip_reader_get_num_files(&zip);
    ESP_LOGI(TAG, "ZIP contains %d entries", file_count);

    /* Create destination directory */
    mkdir(dest_dir, 0755);

    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip, i, &file_stat)) {
            ESP_LOGW(TAG, "Failed to stat entry %d", i);
            continue;
        }

        /* Skip directories */
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            char dir_path[256];
            snprintf(dir_path, sizeof(dir_path), "%s/%s", dest_dir, file_stat.m_filename);
            mkdir(dir_path, 0755);
            continue;
        }

        /* Extract file */
        char dest_path[256];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, file_stat.m_filename);

        /* Ensure parent directory exists */
        char *last_slash = strrchr(dest_path, '/');
        if (last_slash) {
            char parent[256];
            strncpy(parent, dest_path, last_slash - dest_path);
            parent[last_slash - dest_path] = '\0';
            mkdir(parent, 0755);
        }

        if (!mz_zip_reader_extract_to_file(&zip, i, dest_path, 0)) {
            ESP_LOGW(TAG, "Failed to extract: %s", file_stat.m_filename);
        } else {
            ESP_LOGI(TAG, "Extracted: %s (%lu bytes)", file_stat.m_filename,
                     (unsigned long)file_stat.m_uncomp_size);
        }
    }

    mz_zip_reader_end(&zip);
    return true;
}

static bool extract_app(const char *app_path, const char *package_name,
                        char *out_path, size_t out_len)
{
    snprintf(out_path, out_len, "/tmp/%s", package_name);

    /* Clean previous extraction */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", out_path);
    system(cmd);

    ESP_LOGI(TAG, "Extracting %s -> %s", app_path, out_path);

    if (!extract_app_zip(app_path, out_path)) {
        ESP_LOGE(TAG, "ZIP extraction failed");
        return false;
    }

    /* Verify manifest.json exists */
    char manifest_path[256];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", out_path);
    if (access(manifest_path, F_OK) != 0) {
        ESP_LOGE(TAG, "manifest.json not found in .app");
        return false;
    }

    return true;
}

/* ── MicroPython VM Control ─────────────────────────────────────────────── */

static void *allocate_gc_heap(void)
{
    /* Try PSRAM first, fall back to internal RAM */
    void *heap = heap_caps_malloc(MPY_HEAP_SIZE, MALLOC_CAP_SPIRAM);
    if (!heap) {
        ESP_LOGW(TAG, "PSRAM alloc failed, trying internal RAM");
        heap = heap_caps_malloc(MPY_HEAP_SIZE, MALLOC_CAP_INTERNAL);
    }
    return heap;
}

static bool mpy_vm_init(const char *app_dir)
{
    ESP_LOGI(TAG, "Initializing MicroPython VM for %s", app_dir);

    /* Allocate GC heap */
    s_runtime.gc_heap = allocate_gc_heap();
    if (!s_runtime.gc_heap) {
        ESP_LOGE(TAG, "Failed to allocate GC heap (%d bytes)", MPY_HEAP_SIZE);
        return false;
    }

    /* Initialize stack limit */
    int stack_size = 8 * 1024;
    mp_stack_ctrl_init();
    mp_stack_set_limit(stack_size);

    /* Initialize MicroPython */
    gc_init(s_runtime.gc_heap, (char *)s_runtime.gc_heap + MPY_HEAP_SIZE);
    mp_init();

    /* Set sys.path to include app directory */
    mp_obj_list_init(MP_STATE_VM(mp_path), 0);
    mp_obj_list_append(MP_STATE_VM(mp_path),
                       mp_obj_new_str(app_dir, strlen(app_dir)));
    /* Also add /lib subdirectory if it exists */
    char lib_path[256];
    snprintf(lib_path, sizeof(lib_path), "%s/lib", app_dir);
    if (access(lib_path, F_OK) == 0) {
        mp_obj_list_append(MP_STATE_VM(mp_path),
                           mp_obj_new_str(lib_path, strlen(lib_path)));
    }

    /* Register XiaoMiao hardware modules */
    xiaomiao_modules_register();

    ESP_LOGI(TAG, "MicroPython VM initialized (heap=%dKB)", MPY_HEAP_SIZE / 1024);
    return true;
}

static bool mpy_vm_run_main(const char *app_dir)
{
    char main_path[256];
    snprintf(main_path, sizeof(main_path), "%s/main.py", app_dir);

    ESP_LOGI(TAG, "Executing %s", main_path);

    /* Compile and execute */
    mp_obj_t module_fun;
    int ret = pyexec_file(main_path, &module_fun);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to execute main.py (error=%d)", ret);
        return false;
    }

    ESP_LOGI(TAG, "main.py completed");
    return true;
}

static void mpy_vm_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing MicroPython VM");

    mp_deinit();

    if (s_runtime.gc_heap) {
        free(s_runtime.gc_heap);
        s_runtime.gc_heap = NULL;
    }
}

/* ── Public API ───────────────────────────────────────────────────────── */

bool micropython_init(void)
{
    if (s_runtime.initialized) {
        ESP_LOGW(TAG, "MicroPython runtime already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing MicroPython runtime");

    /* Create /tmp directory for app extraction */
    mkdir("/tmp", 0755);

    s_runtime.initialized = true;
    s_runtime.running = false;
    s_runtime.exit_cb = NULL;
    s_runtime.gc_heap = NULL;

    ESP_LOGI(TAG, "MicroPython runtime initialized");
    return true;
}

bool micropython_launch_app(const app_entry_t *app)
{
    if (!s_runtime.initialized) {
        ESP_LOGE(TAG, "MicroPython runtime not initialized");
        return false;
    }

    if (s_runtime.running) {
        ESP_LOGE(TAG, "Another app is already running: %s", s_runtime.current_app);
        return false;
    }

    if (!app || app->is_system) {
        ESP_LOGE(TAG, "Invalid app or system app cannot be launched via MicroPython");
        return false;
    }

    ESP_LOGI(TAG, "Launching app: %s (%s)", app->display_name, app->package_name);

    /* Step 1: Extract .app to /tmp/<package>/ */
    if (!extract_app(app->app_path, app->package_name,
                     s_runtime.extract_path, sizeof(s_runtime.extract_path))) {
        ESP_LOGE(TAG, "Failed to extract app");
        return false;
    }

    /* Step 2: Initialize VM with app directory */
    if (!mpy_vm_init(s_runtime.extract_path)) {
        ESP_LOGE(TAG, "Failed to initialize VM");
        return false;
    }

    /* Step 3: Mark as running */
    s_runtime.running = true;
    strncpy(s_runtime.current_app, app->package_name,
            sizeof(s_runtime.current_app) - 1);

    /* Step 4: Execute main.py */
    bool success = mpy_vm_run_main(s_runtime.extract_path);

    /* Step 5: Cleanup */
    mpy_vm_deinit();
    s_runtime.running = false;
    s_runtime.current_app[0] = '\0';
    s_runtime.extract_path[0] = '\0';

    /* Step 6: Notify exit */
    if (s_runtime.exit_cb) {
        s_runtime.exit_cb(app->package_name, success ? 0 : -1);
    }

    return success;
}

void micropython_stop_app(void)
{
    if (!s_runtime.running) {
        ESP_LOGW(TAG, "No app is running");
        return;
    }

    ESP_LOGI(TAG, "Stopping app: %s", s_runtime.current_app);

    mpy_vm_deinit();

    s_runtime.running = false;
    s_runtime.current_app[0] = '\0';
    s_runtime.extract_path[0] = '\0';
}

bool micropython_is_running(void)
{
    return s_runtime.running;
}

const char *micropython_get_running_app(void)
{
    if (!s_runtime.running) return NULL;
    return s_runtime.current_app;
}

void micropython_set_exit_callback(micropython_exit_cb_t cb)
{
    s_runtime.exit_cb = cb;
}
