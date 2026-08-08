#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of installed apps */
#define XOS_MAX_APPS 64
#define XOS_MAX_NAME_LEN 64

/* App manifest structure (parsed from manifest.json) */
typedef struct {
    char package_name[XOS_MAX_NAME_LEN];
    char display_name[XOS_MAX_NAME_LEN];
    char description[128];
    char author[64];
    char version[32];
    char entry_point[64];
    char icon[64];
    char permissions[128];
    char required_api_version[16];
} xos_app_manifest_t;

/* Installed app info */
typedef struct {
    char filename[XOS_MAX_NAME_LEN];  /* .app filename on SD card */
    xos_app_manifest_t manifest;
    bool valid;
} xos_app_info_t;

/* App manager state */
typedef struct {
    xos_app_info_t apps[XOS_MAX_APPS];
    int count;
} xos_app_list_t;

/* Initialize app manager */
esp_err_t xos_appmgr_init(void);

/* Scan /apps/ directory for .app files and parse manifests */
esp_err_t xos_appmgr_scan(xos_app_list_t *list);

/* Get current app list (cached) */
const xos_app_list_t *xos_appmgr_get_list(void);

/* Install an app from a .app file path */
esp_err_t xos_appmgr_install(const char *app_file_path);

/* Uninstall an app by package name */
esp_err_t xos_appmgr_uninstall(const char *package_name);

/* Launch an app by index (for future MicroPython integration) */
esp_err_t xos_appmgr_launch(int index);

#ifdef __cplusplus
}
#endif