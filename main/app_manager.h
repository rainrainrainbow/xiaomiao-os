/*
 * app_manager.h — Installed .app registry & lifecycle
 *
 * An .app file is a ZIP containing manifest.json + icon.png + main.py [+ lib/ + assets/].
 * The manager scans /sdcard/apps/*.app, parses manifest.json, and maintains
 * an in-memory + Flash-persisted (desktop.json on /vfs) list of installed apps.
 *
 * The registry is a single global instance (s_app_registry) to keep the API
 * simple and avoid passing the struct around. UI modules access it via the
 * query functions below.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define APP_MAX_COUNT        32
#define APP_ID_MAX_LEN       64
#define APP_NAME_MAX_LEN     32
#define APP_VERSION_MAX_LEN  16
#define APP_PATH_MAX_LEN     128

typedef enum {
    APP_KIND_SYSTEM = 0,  /* built-in: applist/settings/editor/store */
    APP_KIND_USER   = 1,  /* installed .app on SD card */
} app_kind_t;

typedef struct {
    app_kind_t kind;
    char       package_name[APP_ID_MAX_LEN];   /* e.g. com.example.myapp, or "sys.settings" */
    char       display_name[APP_NAME_MAX_LEN]; /* UI label */
    char       version[APP_VERSION_MAX_LEN];
    char       entry_point[32];                /* e.g. main.py */
    char       app_path[APP_PATH_MAX_LEN];     /* /sdcard/apps/xxx.app, empty for system */
    char       data_path[APP_PATH_MAX_LEN];    /* /sdcard/data/<package>/ */
    uint8_t    icon_glyph;                     /* single ASCII char (e.g. 'A', 'S') */
    char       icon_str[8];                    /* short emoji/symbol string; if non-empty, icon_str is rendered */
    bool       valid;
    bool       is_system;                      /* convenience alias for kind == APP_KIND_SYSTEM */
    uint32_t   file_size;                      /* size of .app on disk, 0 for system */
} app_entry_t;

typedef struct {
    app_entry_t apps[APP_MAX_COUNT];
    int         count;        /* total (system + user) */
    int         sys_count;    /* number of system apps at the front */
    int         order[APP_MAX_COUNT]; /* permutation of indices; persists user reordering */
} app_registry_t;

/* Access global registry (used by UI components). */
app_registry_t *app_manager_get_registry(void);

/* Registry lifecycle */
void app_manager_init(void);                 /* mounts registry, loads desktop.json order */
int  app_manager_scan_sdcard(void);          /* scans /sdcard/apps/*.app, returns count found */
int  app_manager_count(void);
const app_entry_t *app_manager_get(int index);
const app_entry_t *app_manager_find(const char *package_name);

/* Desktop ordering persistence (desktop.json on /vfs) */
void app_manager_save_order(void);
void app_manager_load_order(void);
void app_manager_move(int from_index, int to_index);

/* Install / uninstall (SD-card copy + registry update) */
bool app_manager_install_from_path(const char *app_file_path);
bool app_manager_uninstall(const char *package_name);

/* Launch: hands off to MicroPython runtime */
bool app_manager_launch(const char *package_name);
bool app_manager_launch_index(int index);