#pragma once
#include "esp_err.h"

/* .app 文件格式 = ZIP 压缩包
 *
 * 内部结构:
 *   manifest.json  (必需) - 应用清单
 *   main.py        (必需) - MicroPython 入口
 *   icon.png       (可选) - 应用图标
 *   lib/           (可选) - 私有库
 *   assets/        (可选) - 资源文件
 *   model/         (可选) - ML 模型
 *
 * manifest.json 字段:
 *   package_name  - 唯一包名 (com.example.app)
 *   version       - 版本号
 *   display_name  - 显示名称
 *   description   - 描述
 *   author        - 作者
 *   entry_point   - 入口文件 (默认 main.py)
 *   icon          - 图标文件 (默认 icon.png)
 *   permissions   - 权限列表
 *   required_api_version - 最低 API 版本
 */

typedef struct {
    char     package[64];
    char     version[16];
    char     display_name[32];
    char     description[128];
    char     author[32];
    char     entry_point[64];
    char     icon[64];
    char     permissions[256];
    char     api_version[16];
} app_manifest_t;

/* 解析 .app 文件 */
esp_err_t app_format_parse(const char *app_path, app_manifest_t *manifest);

/* 解压 .app 到目标目录 */
esp_err_t app_format_extract(const char *app_path, const char *dest_dir);

/* 创建 .app 文件 */
esp_err_t app_format_create(const char *src_dir, const char *output_path);

/* 验证 .app 完整性 */
bool      app_format_verify(const char *app_path);
