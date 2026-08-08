/*
 * app_manager.c — Application Manager for Xiaomiao OS
 *
 * Scans SD card /apps/ directory for .app (ZIP) files,
 * parses manifest.json from each, and manages install/uninstall.
 */

#include "app_manager.h"
#include "xiaomiao_hal.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_log.h"

static const char *TAG = "XOS_APPMGR";

static xos_app_list_t s_app_list;

/* Simple JSON string field extractor (very lightweight, no full parser) */
static void json_extract_string(const char *json, const char *key, char *out, size_t out_len)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(pattern);
    /* skip : " */
    p = strchr(p, ':');
    if (!p) { out[0] = '\0'; return; }
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') { out[0] = '\0'; return; }
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i < out_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

esp_err_t xos_appmgr_init(void)
{
    memset(&s_app_list, 0, sizeof(s_app_list));
    ESP_LOGI(TAG, "App manager initialized");
    return ESP_OK;
}

esp_err_t xos_appmgr_scan(xos_app_list_t *list)
{
    if (!list) list = &s_app_list;
    memset(list, 0, sizeof(*list));

    DIR *dir = opendir(XOS_APPS_DIR);
    if (!dir) {
        ESP_LOGW(TAG, "Cannot open %s", XOS_APPS_DIR);
        return ESP_ERR_NOT_FOUND;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && list->count < XOS_MAX_APPS) {
        /* Look for .app files */
        const char *ext = strrchr(ent->d_name, '.');
        if (!ext || strcmp(ext, ".app") != 0) continue;

        /* For now, just record filename — ZIP parsing will be added in Phase 3 */
        strncpy(list->apps[list->count].filename, ent->d_name,
                XOS_MAX_NAME_LEN - 1);

        /* Try to read manifest from the .app file (ZIP format)
         * For Phase 1, we just mark as valid if file exists */
        list->apps[list->count].valid = true;

        /* TODO: In Phase 3, unzip and parse manifest.json */

        list->count++;
    }
    closedir(dir);

    ESP_LOGI(TAG, "Scanned %s: found %d apps", XOS_APPS_DIR, list->count);
    return ESP_OK;
}

const xos_app_list_t *xos_appmgr_get_list(void)
{
    return &s_app_list;
}

esp_err_t xos_appmgr_install(const char *app_file_path)
{
    /* TODO: Implement in Phase 3 — copy .app file to /apps/ directory */
    ESP_LOGI(TAG, "Install: %s (not yet implemented)", app_file_path);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t xos_appmgr_uninstall(const char *package_name)
{
    /* TODO: Implement in Phase 3 */
    ESP_LOGI(TAG, "Uninstall: %s (not yet implemented)", package_name);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t xos_appmgr_launch(int index)
{
    /* TODO: Implement in Phase 3 with MicroPython integration */
    if (index < 0 || index >= s_app_list.count) return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "Launch: %s (not yet implemented)", s_app_list.apps[index].filename);
    return ESP_ERR_NOT_SUPPORTED;
}