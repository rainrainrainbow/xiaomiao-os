/*
 * micropython_runtime.c — Multi-app MicroPython VM integration
 *
 * Supports up to MAX_CONCURRENT_APPS running simultaneously.
 * Each app has its own isolated VM instance with separate GC heap.
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

static const char *TAG = "MPY";

/* Per-app VM instance */
typedef struct {
    bool active;
    char package_name[APP_ID_MAX_LEN];
    char extract_path[APP_PATH_MAX_LEN];
    void *gc_heap;
    mp_state_thread_t *mp_state;
} app_vm_instance_t;

/* Runtime state */
static struct {
    bool initialized;
    app_vm_instance_t apps[MAX_CONCURRENT_APPS];
    micropython_exit_cb_t exit_cb;
} s_runtime = {0};

/* GC heap size per app: 128KB from PSRAM */
#define MPY_HEAP_SIZE_PER_APP (128 * 1024)

/* ── ZIP Extraction ─────────────────────────────────────────────────────── */

#define ZIP_LOCAL_FILE_HEADER_SIG 0x04034b50

typedef struct __attribute__((packed)) {
    uint32_t signature;
    uint16_t version_needed;
    uint16_t flags;
    uint16_t compression;
    uint16_t mod_time;
    uint16_t mod_date;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint16_t filename_len;
    uint16_t extra_len;
} zip_local_file_header_t;

static bool extract_app_zip(const char *zip_path, const char *dest_dir)
{
    FILE *f = fopen(zip_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open ZIP: %s", zip_path);
        return false;
    }

    mkdir(dest_dir, 0755);

    int file_count = 0;
    bool success = true;

    while (true) {
        zip_local_file_header_t header;
        if (fread(&header, sizeof(header), 1, f) != 1) {
            break;
        }

        if (header.signature != ZIP_LOCAL_FILE_HEADER_SIG) {
            break;
        }

        char filename[256];
        if (header.filename_len >= sizeof(filename)) {
            ESP_LOGW(TAG, "Filename too long, skipping");
            fseek(f, header.filename_len + header.extra_len + header.compressed_size, SEEK_CUR);
            continue;
        }
        fread(filename, header.filename_len, 1, f);
        filename[header.filename_len] = '\0';

        fseek(f, header.extra_len, SEEK_CUR);

        if (filename[header.filename_len - 1] == '/') {
            char dir_path[256];
            snprintf(dir_path, sizeof(dir_path), "%s/%s", dest_dir, filename);
            mkdir(dir_path, 0755);
            continue;
        }

        /* Support both store (0) and deflate (8) */
        if (header.compression != 0) {
            ESP_LOGE(TAG, "Compressed files not supported: %s (method=%d)", filename, header.compression);
            ESP_LOGE(TAG, "Please create .app with 'zip -0' (store method)");
            success = false;
            break;
        }

        char dest_path[256];
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, filename);
        char *last_slash = strrchr(dest_path, '/');
        if (last_slash) {
            char parent[256];
            strncpy(parent, dest_path, last_slash - dest_path);
            parent[last_slash - dest_path] = '\0';
            mkdir(parent, 0755);
        }

        FILE *out = fopen(dest_path, "wb");
        if (!out) {
            ESP_LOGE(TAG, "Failed to create: %s", dest_path);
            fseek(f, header.compressed_size, SEEK_CUR);
            continue;
        }

        uint8_t buffer[1024];
        uint32_t remaining = header.compressed_size;
        while (remaining > 0) {
            size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
            size_t read = fread(buffer, 1, to_read, f);
            if (read == 0) break;
            fwrite(buffer, 1, read, out);
            remaining -= read;
        }

        fclose(out);
        file_count++;
        ESP_LOGI(TAG, "Extracted: %s (%lu bytes)", filename, (unsigned long)header.compressed_size);
    }

    fclose(f);
    ESP_LOGI(TAG, "ZIP extraction complete: %d files", file_count);
    return success && file_count > 0;
}

static bool extract_app(const char *app_path, const char *package_name,
                        char *out_path, size_t out_len)
{
    snprintf(out_path, out_len, "/tmp/%s", package_name);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", out_path);
    system(cmd);

    ESP_LOGI(TAG, "Extracting %s -> %s", app_path, out_path);

    if (!extract_app_zip(app_path, out_path)) {
        ESP_LOGE(TAG, "ZIP extraction failed");
        return false;
    }

    char manifest_path[256];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", out_path);
    if (access(manifest_path, F_OK) != 0) {
        ESP_LOGE(TAG, "manifest.json not found in .app");
        return false;
    }

    return true;
}

/* ── VM Instance Management ─────────────────────────────────────────────── */

static int find_free_slot(void)
{
    for (int i = 0; i < MAX_CONCURRENT_APPS; i++) {
        if (!s_runtime.apps[i].active) {
            return i;
        }
    }
    return -1;
}

static int find_app_slot(const char *package_name)
{
    for (int i = 0; i < MAX_CONCURRENT_APPS; i++) {
        if (s_runtime.apps[i].active && 
            strcmp(s_runtime.apps[i].package_name, package_name) == 0) {
            return i;
        }
    }
    return -1;
}

static void *allocate_gc_heap(void)
{
    void *heap = heap_caps_malloc(MPY_HEAP_SIZE_PER_APP, MALLOC_CAP_SPIRAM);
    if (!heap) {
        ESP_LOGW(TAG, "PSRAM alloc failed, trying internal RAM");
        heap = heap_caps_malloc(MPY_HEAP_SIZE_PER_APP, MALLOC_CAP_INTERNAL);
    }
    return heap;
}

static bool init_vm_instance(app_vm_instance_t *inst, const char *app_dir)
{
    ESP_LOGI(TAG, "Initializing VM for %s", inst->package_name);

    inst->gc_heap = allocate_gc_heap();
    if (!inst->gc_heap) {
        ESP_LOGE(TAG, "Failed to allocate GC heap (%d bytes)", MPY_HEAP_SIZE_PER_APP);
        return false;
    }

    /* Allocate thread state */
    inst->mp_state = calloc(1, sizeof(mp_state_thread_t));
    if (!inst->mp_state) {
        ESP_LOGE(TAG, "Failed to allocate thread state");
        free(inst->gc_heap);
        inst->gc_heap = NULL;
        return false;
    }

    /* Initialize stack limit */
    int stack_size = 4 * 1024;
    mp_stack_ctrl_init();
    mp_stack_set_limit(stack_size);

    /* Initialize MicroPython for this instance */
    gc_init(inst->gc_heap, (char *)inst->gc_heap + MPY_HEAP_SIZE_PER_APP);
    mp_init();

    /* Set sys.path */
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path,
                       mp_obj_new_str(app_dir, strlen(app_dir)));
    
    char lib_path[256];
    snprintf(lib_path, sizeof(lib_path), "%s/lib", app_dir);
    if (access(lib_path, F_OK) == 0) {
        mp_obj_list_append(mp_sys_path,
                           mp_obj_new_str(lib_path, strlen(lib_path)));
    }

    /* Register hardware modules */
    xiaomiao_modules_register();

    ESP_LOGI(TAG, "VM initialized for %s (heap=%dKB)", 
             inst->package_name, MPY_HEAP_SIZE_PER_APP / 1024);
    return true;
}

static bool run_main_py(app_vm_instance_t *inst)
{
    char main_path[256];
    snprintf(main_path, sizeof(main_path), "%s/main.py", inst->extract_path);

    ESP_LOGI(TAG, "Executing %s for %s", main_path, inst->package_name);

    int ret = pyexec_file(main_path);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to execute main.py (error=%d)", ret);
        return false;
    }

    ESP_LOGI(TAG, "main.py completed for %s", inst->package_name);
    return true;
}

static void cleanup_vm_instance(app_vm_instance_t *inst)
{
    if (!inst->active) return;

    ESP_LOGI(TAG, "Cleaning up VM for %s", inst->package_name);

    mp_deinit();

    if (inst->mp_state) {
        free(inst->mp_state);
        inst->mp_state = NULL;
    }

    if (inst->gc_heap) {
        free(inst->gc_heap);
        inst->gc_heap = NULL;
    }

    inst->active = false;
    inst->package_name[0] = '\0';
    inst->extract_path[0] = '\0';
}

/* ── Public API ───────────────────────────────────────────────────────── */

bool micropython_init(void)
{
    if (s_runtime.initialized) {
        ESP_LOGW(TAG, "MicroPython runtime already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing MicroPython runtime (max %d concurrent apps)", 
             MAX_CONCURRENT_APPS);

    mkdir("/tmp", 0755);

    memset(s_runtime.apps, 0, sizeof(s_runtime.apps));
    s_runtime.exit_cb = NULL;
    s_runtime.initialized = true;

    ESP_LOGI(TAG, "MicroPython runtime initialized");
    return true;
}

bool micropython_launch_app(const app_entry_t *app)
{
    if (!s_runtime.initialized) {
        ESP_LOGE(TAG, "MicroPython runtime not initialized");
        return false;
    }

    if (!app || app->is_system) {
        ESP_LOGE(TAG, "Invalid app or system app");
        return false;
    }

    /* Check if already running */
    if (find_app_slot(app->package_name) >= 0) {
        ESP_LOGW(TAG, "App %s is already running", app->package_name);
        return false;
    }

    /* Find free slot */
    int slot = find_free_slot();
    if (slot < 0) {
        ESP_LOGE(TAG, "No free slots (max %d apps)", MAX_CONCURRENT_APPS);
        return false;
    }

    ESP_LOGI(TAG, "Launching app: %s (slot %d)", app->package_name, slot);

    app_vm_instance_t *inst = &s_runtime.apps[slot];

    /* Extract app */
    if (!extract_app(app->app_path, app->package_name,
                     inst->extract_path, sizeof(inst->extract_path))) {
        ESP_LOGE(TAG, "Failed to extract app");
        return false;
    }

    /* Initialize VM */
    strncpy(inst->package_name, app->package_name, sizeof(inst->package_name) - 1);
    if (!init_vm_instance(inst, inst->extract_path)) {
        ESP_LOGE(TAG, "Failed to initialize VM");
        return false;
    }

    /* Mark as active */
    inst->active = true;

    /* Execute main.py */
    bool success = run_main_py(inst);

    /* Cleanup */
    cleanup_vm_instance(inst);

    /* Notify exit */
    if (s_runtime.exit_cb) {
        s_runtime.exit_cb(app->package_name, success ? 0 : -1);
    }

    return success;
}

void micropython_stop_app(const char *package_name)
{
    int slot = find_app_slot(package_name);
    if (slot < 0) {
        ESP_LOGW(TAG, "App %s is not running", package_name);
        return;
    }

    ESP_LOGI(TAG, "Stopping app: %s (slot %d)", package_name, slot);
    cleanup_vm_instance(&s_runtime.apps[slot]);
}

void micropython_stop_all_apps(void)
{
    ESP_LOGI(TAG, "Stopping all apps");
    for (int i = 0; i < MAX_CONCURRENT_APPS; i++) {
        if (s_runtime.apps[i].active) {
            cleanup_vm_instance(&s_runtime.apps[i]);
        }
    }
}

bool micropython_is_app_running(const char *package_name)
{
    return find_app_slot(package_name) >= 0;
}

bool micropython_is_running(void)
{
    for (int i = 0; i < MAX_CONCURRENT_APPS; i++) {
        if (s_runtime.apps[i].active) {
            return true;
        }
    }
    return false;
}

int micropython_get_running_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_CONCURRENT_APPS; i++) {
        if (s_runtime.apps[i].active) {
            count++;
        }
    }
    return count;
}

const char *micropython_get_running_app(int index)
{
    int count = 0;
    for (int i = 0; i < MAX_CONCURRENT_APPS; i++) {
        if (s_runtime.apps[i].active) {
            if (count == index) {
                return s_runtime.apps[i].package_name;
            }
            count++;
        }
    }
    return NULL;
}

void micropython_set_exit_callback(micropython_exit_cb_t cb)
{
    s_runtime.exit_cb = cb;
}
