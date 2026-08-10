// ================ sdcard.h - TF 卡 SDSPI 驱动 ================
// 共享 SPI2 与 LCD, CS=22

#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_PIN_CS   22
#define SD_MOUNT_POINT "/sdcard"

esp_err_t sdcard_init(void);
void sdcard_deinit(void);
bool sdcard_is_mounted(void);
uint64_t sdcard_total_bytes(void);
uint64_t sdcard_free_bytes(void);

#ifdef __cplusplus
}
#endif

#endif // __SDCARD_H__