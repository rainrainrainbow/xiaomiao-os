// ================ app_loader.h - 从 SD 卡加载 .app 包 ================
// .app 格式:
//
//   {
//     "id": "com.demo.snake",
//     "name": "贪吃蛇",
//     "glyph": "🐍",
//     "version": "1.0.2",
//     "type": "blocks",  // 或 "native" (TODO)
//     "blocks": ["延时(500)", "..."]
//   }
//
// 存储路径: /sdcard/roms/*.app

#ifndef __APP_LOADER_H__
#define __APP_LOADER_H__

#include "core/app_manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 启动扫描 — 把 /sdcard/roms/ 下所有 .app 注册成新 App */
int app_loader_scan_app_dir(void);

/** 加载单个 .app JSON 文件到 app_info_t */
int app_loader_load_file(const char *path, app_info_t *out);

/** 把积木数组生成 main.py 字符串到 buf */
int app_loader_blocks_to_python(const char *const *blocks, int n,
                                char *buf, int buf_sz);

/** 从 .app 包文件里读 main.py 源码（PSRAM 分配, 调用方 free） */
char *app_loader_read_source(const char *path);

/** 扫描 /sdcard/roms/ 下所有 .app, 对每个回调 */
int app_loader_scan_app_dir_with_cb(int (*cb)(const app_info_t *));

#ifdef __cplusplus
}
#endif

#endif // __APP_LOADER_H__