/*
 * app_format.c - .app 文件格式处理
 *
 * .app 文件本质上是 ZIP 压缩包
 * 使用 ESP-IDF 内置的 minizip 或 zlib 解压
 *
 * 当前为接口实现，完整版需链接 unzip 库
 */
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#include "app_runtime/app_format.h"

static const char *TAG = "app_format";

/* ---------- 解析 manifest.json ---------- */
esp_err_t app_format_parse(const char *app_path, app_manifest_t *manifest)
{
    if (!app_path || !manifest) return ESP_ERR_INVALID_ARG;

    ESP_LOGI(TAG, "Parsing: %s", app_path);

    /* 尝试直接读取解压后的 manifest.json */
    char dir_path[200];
    snprintf(dir_path, sizeof(dir_path), "%s", app_path);

    /* 去掉 .app 后缀得到目录 */
    char *dot = strrchr(dir_path, '.');
    if (dot && strcmp(dot, ".app") == 0) *dot = '\0';

    char manifest_path[256];
    snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json", dir_path);

    FILE *f = fopen(manifest_path, "r");
    if (!f) {
        ESP_LOGE(TAG, "manifest.json not found in %s", dir_path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return ESP_ERR_NO_MEM; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    /* 简易 JSON 解析 */
    memset(manifest, 0, sizeof(*manifest));

    /* package_name */
    char *p = strstr(buf, "\"package_name\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 63) manifest->package[i++] = *p++;
    }
    /* display_name */
    p = strstr(buf, "\"display_name\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 31) manifest->display_name[i++] = *p++;
    }
    /* version */
    p = strstr(buf, "\"version\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 15) manifest->version[i++] = *p++;
    }
    /* description */
    p = strstr(buf, "\"description\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 127) manifest->description[i++] = *p++;
    }
    /* author */
    p = strstr(buf, "\"author\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 31) manifest->author[i++] = *p++;
    }
    /* entry_point */
    p = strstr(buf, "\"entry_point\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 63) manifest->entry_point[i++] = *p++;
    }
    /* icon */
    p = strstr(buf, "\"icon\"");
    if (p) {
        p = strchr(p, ':'); p++;
        while (*p == ' ' || *p == '"') p++;
        int i = 0; while (*p && *p != '"' && i < 63) manifest->icon[i++] = *p++;
    }

    free(buf);

    ESP_LOGI(TAG, "  Package: %s", manifest->package);
    ESP_LOGI(TAG, "  Name: %s", manifest->display_name);
    ESP_LOGI(TAG, "  Version: %s", manifest->version);
    ESP_LOGI(TAG, "  Entry: %s", manifest->entry_point);

    return ESP_OK;
}

esp_err_t app_format_extract(const char *app_path, const char *dest_dir)
{
    ESP_LOGI(TAG, "Extracting %s -> %s", app_path, dest_dir);
    /* TODO: 调用 unzip 库解压 ZIP */
    /* 需要链接 minizip 或 libarchive */
    return ESP_OK;
}

esp_err_t app_format_create(const char *src_dir, const char *output_path)
{
    ESP_LOGI(TAG, "Creating %s from %s", output_path, src_dir);
    /* TODO: 调用 zip 库打包 */
    return ESP_OK;
}

bool app_format_verify(const char *app_path)
{
    FILE *f = fopen(app_path, "rb");
    if (!f) return false;

    uint8_t header[4];
    fread(header, 1, 4, f);
    fclose(f);

    /* ZIP 头: PK\x03\x04 */
    return (header[0] == 'P' && header[1] == 'K' &&
            header[2] == 0x03 && header[3] == 0x04);
}
