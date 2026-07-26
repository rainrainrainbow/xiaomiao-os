/*
 * app_manager.c - App 管理器
 *
 * 扫描 /sdcard/apps/ 目录下的 .app 文件
 * 解析 manifest.json，注册到全局应用列表
 *
 * .app 文件结构 (ZIP):
 *   manifest.json   - 应用清单
 *   main.py         - 入口脚本
 *   icon.png        - 图标
 *   lib/            - 私有库
 *   assets/         - 资源文件
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"

#include "app_runtime/app_manager.h"
#include "desktop/desktop.h"
#include "ui/theme.h"

static const char *TAG = "app_manager";

/* 简易 ZIP 解析（查找中央目录） */
typedef struct {
    char name[64];
    uint32_t offset;   /* 在 ZIP 中的偏移 */
    uint32_t size;      /* 解压后大小 */
} zip_entry_t;

#define MAX_ZIP_ENTRIES 32
static zip_entry_t s_zip_entries[MAX_ZIP_ENTRIES];
static int        s_zip_count = 0;

/* ---------- 内部函数 ---------- */
static bool parse_manifest(const char *json, app_item_t *app)
{
    /* 简易 JSON 解析（找关键字段） */
    /* 完整版应使用 cJSON，这里做最小匹配 */
    const char *p;

    p = strstr(json, "\"package_name\"");
    if (p) {
        p = strchr(p, ':'); if (!p) return false;
        p++; while (*p == ' ' || *p == '"') p++;
        int i = 0;
        while (*p && *p != '"' && i < 63) app->package[i++] = *p++;
        app->package[i] = '\0';
    }

    p = strstr(json, "\"display_name\"");
    if (p) {
        p = strchr(p, ':'); if (!p) return false;
        p++; while (*p == ' ' || *p == '"') p++;
        int i = 0;
        while (*p && *p != '"' && i < 31) app->name[i++] = *p++;
        app->name[i] = '\0';
    }

    p = strstr(json, "\"icon\"");
    if (p) {
        p = strchr(p, ':'); if (!p) return false;
        p++; while (*p == ' ' || *p == '"') p++;
        int i = 0;
        while (*p && *p != '"' && i < 127) app->icon_path[i++] = *p++;
        app->icon_path[i] = '\0';
    }

    p = strstr(json, "\"entry_point\"");
    if (p) {
        p = strchr(p, ':'); if (!p) return false;
        p++; while (*p == ' ' || *p == '"') p++;
        int i = 0;
        while (*p && *p != '"' && i < 127) app->entry_path[i++] = *p++;
        app->entry_path[i] = '\0';
    }

    app->is_system = false;
    app->is_block_app = false;
    app->tile_color = ui_tile_color(g_app_count);
    return true;
}

static void scan_apps_dir(void)
{
    DIR *dir = opendir("/sdcard/apps");
    if (!dir) {
        ESP_LOGW(TAG, "No /sdcard/apps directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) && g_app_count < MAX_APPS) {
        if (strstr(entry->d_name, ".app") || strstr(entry->d_name, ".zip")) {
            char path[160];
            snprintf(path, sizeof(path), "/sdcard/apps/%s", entry->d_name);

            /* 读取 ZIP 中的 manifest.json */
            FILE *f = fopen(path, "rb");
            if (!f) continue;

            /* 简易: 找 "manifest.json" 字符串定位 */
            fseek(f, 0, SEEK_END);
            long fsize = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *buf = malloc(fsize + 1);
            if (buf) {
                fread(buf, 1, fsize, f);
                buf[fsize] = '\0';

                /* 检查是否是 ZIP (PK 头) */
                if (fsize > 4 && buf[0] == 'P' && buf[1] == 'K') {
                    /* ZIP 文件 - 提取 manifest */
                    /* 简化: 不解析 ZIP 结构，假设已解压 */
                    ESP_LOGD(TAG, "Found .app: %s", entry->d_name);
                } else {
                    /* 可能是解压后的目录 */
                }
                free(buf);
            }
            fclose(f);

            /* 检查解压目录 */
            char dir_path[160];
            snprintf(dir_path, sizeof(dir_path), "/sdcard/apps/%s", entry->d_name);
            /* 去掉 .app 后缀 */
            char *dot = strrchr(dir_path, '.');
            if (dot) *dot = '\0';

            char manifest_path[200];
            snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", dir_path);

            FILE *mf = fopen(manifest_path, "r");
            if (mf) {
                fseek(mf, 0, SEEK_END);
                long msize = ftell(mf);
                fseek(mf, 0, SEEK_SET);
                char *mjson = malloc(msize + 1);
                if (mjson) {
                    fread(mjson, 1, msize, mf);
                    mjson[msize] = '\0';

                    if (parse_manifest(mjson, &g_apps[g_app_count])) {
                        snprintf(g_apps[g_app_count].entry_path, 128,
                                 "%s/%s", dir_path, g_apps[g_app_count].entry_path);
                        ESP_LOGI(TAG, "  App: %s (%s)", g_apps[g_app_count].name,
                                 g_apps[g_app_count].package);
                        g_app_count++;
                    }
                    free(mjson);
                }
                fclose(mf);
            }
        }
    }
    closedir(dir);
}

/* ---------- 公开 API ---------- */
esp_err_t app_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing App Manager...");
    g_app_count = 0;
    memset(g_apps, 0, sizeof(g_apps));
    return ESP_OK;
}

void app_manager_scan(void)
{
    g_app_count = 0;
    scan_apps_dir();
    ESP_LOGI(TAG, "Scan complete: %d apps found", g_app_count);
}

int app_manager_get_count(void) { return g_app_count; }
app_item_t *app_manager_get(int index)
{
    if (index < 0 || index >= g_app_count) return NULL;
    return &g_apps[index];
}

esp_err_t app_manager_install(const char *app_path)
{
    /* TODO: 解压 .app(zip) 到 /sdcard/apps/<package>/ */
    ESP_LOGI(TAG, "Installing: %s", app_path);
    app_manager_scan();
    return ESP_OK;
}

esp_err_t app_manager_uninstall(const char *package)
{
    /* TODO: 删除 /sdcard/apps/<package>/ 目录 */
    ESP_LOGI(TAG, "Uninstalling: %s", package);
    app_manager_scan();
    return ESP_OK;
}

esp_err_t app_runtime_launch_app(app_item_t *app)
{
    if (!app) return ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "Launching app: %s", app->name);
    /* 执行入口脚本 */
    /* mp_engine_exec_file(app->entry_path); */
    return ESP_OK;
}

esp_err_t app_runtime_launch_system(const char *app_name)
{
    ESP_LOGI(TAG, "Launching system app: %s", app_name);
    /* 系统内置应用运行逻辑 */
    return ESP_OK;
}

void app_runtime_stop(void)
{
    ESP_LOGI(TAG, "App runtime stopped");
}

bool app_runtime_is_running(void) { return false; }
