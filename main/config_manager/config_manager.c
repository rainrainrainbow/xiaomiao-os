/**
 * @file config_manager.c
 * @brief 配置管理器 - JSON配置持久化
 */

#include "config_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

static const char *TAG = "config_manager";

// 配置存储
static cJSON *s_config = NULL;
static bool s_initialized = false;
static bool s_dirty = false;  // 标记是否有未保存的更改

// 创建配置目录
static esp_err_t ensure_config_dir(void)
{
    // 创建 .xiaomiao 目录
    struct stat st;
    if (stat("/sdcard/.xiaomiao", &st) != 0) {
        if (mkdir("/sdcard/.xiaomiao", 0755) != 0) {
            ESP_LOGE(TAG, "Failed to create config directory: %s", strerror(errno));
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Created config directory");
    }
    return ESP_OK;
}

esp_err_t config_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing config manager");
    
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    // 确保配置目录存在
    esp_err_t ret = ensure_config_dir();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 创建空配置对象
    s_config = cJSON_CreateObject();
    if (!s_config) {
        ESP_LOGE(TAG, "Failed to create config object");
        return ESP_ERR_NO_MEM;
    }
    
    s_initialized = true;
    
    // 尝试加载配置
    ret = config_manager_load();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No config file found, using defaults");
    }
    
    ESP_LOGI(TAG, "Config manager initialized");
    return ESP_OK;
}

esp_err_t config_manager_load(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Loading config from %s", CONFIG_FILE_PATH);
    
    FILE *f = fopen(CONFIG_FILE_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "Config file not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize <= 0 || fsize > 64 * 1024) {  // 限制64KB
        ESP_LOGE(TAG, "Invalid config file size: %ld", fsize);
        fclose(f);
        return ESP_FAIL;
    }
    
    // 读取文件内容
    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    
    size_t read_size = fread(json_str, 1, fsize, f);
    fclose(f);
    
    if (read_size != (size_t)fsize) {
        free(json_str);
        return ESP_FAIL;
    }
    json_str[fsize] = '\0';
    
    // 解析JSON
    cJSON *new_config = cJSON_Parse(json_str);
    free(json_str);
    
    if (!new_config) {
        ESP_LOGE(TAG, "Failed to parse config JSON");
        return ESP_FAIL;
    }
    
    // 替换旧配置
    if (s_config) {
        cJSON_Delete(s_config);
    }
    s_config = new_config;
    
    ESP_LOGI(TAG, "Config loaded successfully");
    return ESP_OK;
}

esp_err_t config_manager_save(void)
{
    if (!s_initialized || !s_config) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Saving config to %s", CONFIG_FILE_PATH);
    
    // 生成JSON字符串
    char *json_str = cJSON_Print(s_config);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to generate JSON");
        return ESP_ERR_NO_MEM;
    }
    
    // 写入文件
    FILE *f = fopen(CONFIG_FILE_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open config file for writing");
        free(json_str);
        return ESP_FAIL;
    }
    
    size_t len = strlen(json_str);
    size_t written = fwrite(json_str, 1, len, f);
    fclose(f);
    free(json_str);
    
    if (written != len) {
        ESP_LOGE(TAG, "Failed to write config file");
        return ESP_FAIL;
    }
    
    s_dirty = false;
    ESP_LOGI(TAG, "Config saved successfully");
    return ESP_OK;
}

int config_manager_get_int(const char *key, int default_value)
{
    if (!s_initialized || !s_config || !key) {
        return default_value;
    }
    
    cJSON *item = cJSON_GetObjectItem(s_config, key);
    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }
    
    return default_value;
}

void config_manager_set_int(const char *key, int value)
{
    if (!s_initialized || !s_config || !key) {
        return;
    }
    
    cJSON *item = cJSON_GetObjectItem(s_config, key);
    if (item) {
        cJSON_SetNumberValue(item, value);
    } else {
        cJSON_AddNumberToObject(s_config, key, value);
    }
    
    s_dirty = true;
}

void config_manager_get_str(const char *key, const char *default_value, 
                            char *buffer, size_t buffer_size)
{
    if (!s_initialized || !s_config || !key || !buffer || buffer_size == 0) {
        if (buffer && buffer_size > 0 && default_value) {
            strncpy(buffer, default_value, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
        }
        return;
    }
    
    cJSON *item = cJSON_GetObjectItem(s_config, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strncpy(buffer, item->valuestring, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    } else if (default_value) {
        strncpy(buffer, default_value, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

void config_manager_set_str(const char *key, const char *value)
{
    if (!s_initialized || !s_config || !key) {
        return;
    }
    
    cJSON *item = cJSON_GetObjectItem(s_config, key);
    if (item) {
        if (value) {
            cJSON_SetValuestring(item, value);
        } else {
            cJSON_ReplaceItemInObject(s_config, key, cJSON_CreateNull());
        }
    } else if (value) {
        cJSON_AddStringToObject(s_config, key, value);
    }
    
    s_dirty = true;
}

bool config_manager_get_bool(const char *key, bool default_value)
{
    if (!s_initialized || !s_config || !key) {
        return default_value;
    }
    
    cJSON *item = cJSON_GetObjectItem(s_config, key);
    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    
    return default_value;
}

void config_manager_set_bool(const char *key, bool value)
{
    if (!s_initialized || !s_config || !key) {
        return;
    }
    
    cJSON *item = cJSON_GetObjectItem(s_config, key);
    if (item) {
        cJSON_ReplaceItemInObject(s_config, key, cJSON_CreateBool(value));
    } else {
        cJSON_AddBoolToObject(s_config, key, value);
    }
    
    s_dirty = true;
}

void config_manager_delete(const char *key)
{
    if (!s_initialized || !s_config || !key) {
        return;
    }
    
    cJSON_DeleteItemFromObject(s_config, key);
    s_dirty = true;
}

void config_manager_reset(void)
{
    if (!s_initialized) {
        return;
    }
    
    ESP_LOGI(TAG, "Resetting config to defaults");
    
    if (s_config) {
        cJSON_Delete(s_config);
    }
    
    s_config = cJSON_CreateObject();
    s_dirty = true;
    
    // 保存默认配置
    config_manager_save();
}
