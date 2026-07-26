#pragma once
#include "esp_err.h"
#include "driver/sdmmc_host.h"

#define SDCARD_PIN_CS    22
#define SDCARD_SPI_HOST  SPI2_HOST  /* 与 LCD 共享 SPI2 */
#define SDCARD_MOUNT_POINT "/sdcard"

esp_err_t sdcard_init(void);
void      sdcard_deinit(void);
bool      sdcard_is_mounted(void);
