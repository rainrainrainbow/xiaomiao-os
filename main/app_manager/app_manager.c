/**
 * @file app_manager.c
 * @brief 应用管理器实现
 */

#include "app_manager.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

static const char *TAG = "app_manager";

// 全局应用列表
static app_list_t g_app_list = {0};

// 当前运行的应用
static char g_current_app[64] = {0};

// SD卡挂载点
#define MOUNT_POINT "/sdcard"
#define APPS_DIR "/sdcard/apps"

esp_err_t app_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing app manager");
    
    // 初始化应用列表
    g_app_list.apps = NULL;
    g_app_list.count = 0;
    g_app_list.capacity = 0;
    
    // 清空当前应用
    memset(g_current_app, 0, sizeof(g_current_app));
    
    ESP_LOGI(TAG, "App manager initialized");
    return ESP_OK;
}

esp_err_t app_manager_parse_app(const char *app_path, app_info_t *info)
{
    ESP_LOGI(TAG, "Parsing app: %s", app_path);
    
    // 构建manifest.json路径
    char manifest_path[256];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", app_path);
    
    // 读取manifest.json
    FILE *f = fopen(manifest_path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open manifest.json: %s", manifest_path);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // 读取文件内容
    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    
    size_t read_size = fread(json_str, 1, fsize, f);
    fclose(f);
    
    if (read_size != fsize) {
        free(json_str);
        return ESP_FAIL;
    }
    json_str[fsize] = 0;
    
    // 解析JSON
    cJSON *json = cJSON_Parse(json_str);
    free(json_str);
    
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse manifest.json");
        return ESP_FAIL;
    }
    
    // 提取应用信息
    cJSON *item;
    
    item = cJSON_GetObjectItem(json, "id");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(info->id, item->valuestring, sizeof(info->id) - 1);
    }
    
    item = cJSON_GetObjectItem(json, "name");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(info->name, item->valuestring, sizeof(info->name) - 1);
    }
    
    item = cJSON_GetObjectItem(json, "version");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(info->version, item->valuestring, sizeof(info->version) - 1);
    }
    
    item = cJSON_GetObjectItem(json, "author");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(info->author, item->valuestring, sizeof(info->author) - 1);
    }
    
    item = cJSON_GetObjectItem(json, "description");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        strncpy(info->description, item->valuestring, sizeof(info->description) - 1);
    }
    
    item = cJSON_GetObjectItem(json, "icon");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        snprintf(info->icon_path, sizeof(info->icon_path), "%s/%s", app_path, item->valuestring);
    }
    
    item = cJSON_GetObjectItem(json, "main");
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        snprintf(info->main_script, sizeof(info->main_script), "%s/%s", app_path, item->valuestring);
    }
    
    cJSON_Delete(json);
    
    ESP_LOGI(TAG, "Parsed app: %s (%s)", info->name, info->id);
    return ESP_OK;
}

esp_err_t app_manager_scan_apps(void)
{
    ESP_LOGI(TAG, "Scanning apps in %s", APPS_DIR);
    
    // 使用全局应用列表
    app_list_t *list = &g_app_list;
    
    // 打开apps目录
    DIR *dir = opendir(APPS_DIR);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open apps directory");
        return ESP_ERR_NOT_FOUND;
    }
    
    // 清空列表
    if (list->apps) {
        free(list->apps);
    }
    list->count = 0;
    list->capacity = 16;  // 初始容量
    list->apps = malloc(sizeof(app_info_t) * list->capacity);
    
    if (!list->apps) {
        closedir(dir);
        return ESP_ERR_NO_MEM;
    }
    
    // 遍历目录
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过.和..
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        // 检查是否是目录
        char app_path[1024];  // 增大缓冲区避免截断警告
        snprintf(app_path, sizeof(app_path), "%s/%.500s", APPS_DIR, entry->d_name);
        
        struct stat st;
        if (stat(app_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }
        
        // 检查是否有manifest.json
        char manifest_path[1024];  // 增大缓冲区避免截断警告
        snprintf(manifest_path, sizeof(manifest_path), "%.900s/manifest.json", app_path);
        
        if (stat(manifest_path, &st) != 0) {
            ESP_LOGW(TAG, "Skipping %s: no manifest.json", entry->d_name);
            continue;
        }
        
        // 检查容量
        if (list->count >= list->capacity) {
            list->capacity *= 2;
            app_info_t *new_apps = realloc(list->apps, sizeof(app_info_t) * list->capacity);
            if (!new_apps) {
                closedir(dir);
                return ESP_ERR_NO_MEM;
            }
            list->apps = new_apps;
        }
        
        // 解析应用
        app_info_t *info = &list->apps[list->count];
        memset(info, 0, sizeof(app_info_t));
        
        esp_err_t ret = app_manager_parse_app(app_path, info);
        if (ret == ESP_OK) {
            info->size = st.st_size;
            list->count++;
            ESP_LOGI(TAG, "Found app: %s (%s)", info->name, info->id);
        }
    }
    
    closedir(dir);
    
    ESP_LOGI(TAG, "Scan complete: found %d apps", list->count);
    return ESP_OK;
}

app_list_t *app_manager_get_list(void)
{
    return &g_app_list;
}

esp_err_t app_manager_launch_app(const char *app_id)
{
    ESP_LOGI(TAG, "Launching app: %s", app_id);
    
    // 查找应用
    app_info_t *app = NULL;
    for (uint32_t i = 0; i < g_app_list.count; i++) {
        if (strcmp(g_app_list.apps[i].id, app_id) == 0) {
            app = &g_app_list.apps[i];
            break;
        }
    }
    
    if (!app) {
        ESP_LOGE(TAG, "App not found: %s", app_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 记录当前应用
    strncpy(g_current_app, app_id, sizeof(g_current_app) - 1);
    
    // TODO: 启动MicroPython运行时执行应用
    ESP_LOGI(TAG, "App launched: %s", app->name);
    
    return ESP_OK;
}

esp_err_t app_manager_stop_app(const char *app_id)
{
    ESP_LOGI(TAG, "Stopping app: %s", app_id);
    
    // 检查是否是当前应用
    if (strcmp(g_current_app, app_id) != 0) {
        ESP_LOGW(TAG, "App not running: %s", app_id);
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: 停止MicroPython运行时
    memset(g_current_app, 0, sizeof(g_current_app));
    
    ESP_LOGI(TAG, "App stopped: %s", app_id);
    return ESP_OK;
}

const char *app_manager_get_current_app(void)
{
    if (g_current_app[0] == '\0') {
        return NULL;
    }
    return g_current_app;
}

void app_manager_free_list(app_list_t *list)
{
    if (list->apps) {
        free(list->apps);
        list->apps = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}