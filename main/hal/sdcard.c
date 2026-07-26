/*
 * sdcard.c - MicroSD 卡初始化 (SDSPI over SPI2, 与 LCD 共享总线)
 */
#include <string.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_device.h"
#include "driver/spi_common.h"
#include "hal/sdcard.h"

static const char *TAG = "sdcard";
static bool s_mounted = false;

esp_err_t sdcard_init(void)
{
    ESP_LOGI(TAG, "Initializing SD card on SPI2 (CS=GPIO%d)...", SDCARD_PIN_CS);

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SDCARD_SPI_HOST;
    host.max_freq_khz = 20000;  /* 20MHz，与 LCD 共享总线需保守 */

    sdspi_device_config_t dev_cfg = {
        .host_id = SDCARD_SPI_HOST,
        .gpio_cs = SDCARD_PIN_CS,
        .gpio_cd = -1,
        .gpio_wp = -1,
        .gpio_int = -1,
    };

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card = NULL;
    esp_err_t ret = esp_vfs_fat_sdspi_mount(SDCARD_MOUNT_POINT, &host, &dev_cfg, &mount_cfg, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: %s", esp_err_to_name(ret));
        s_mounted = false;
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted: %s %s (%.1f MB)",
             card->cid.name, card->cid.mfg_id, (float)card->csd.capacity / 2048.0f);
    s_mounted = true;

    /* 确保目录结构存在 */
    mkdir(SDCARD_MOUNT_POINT "/apps", 0777);
    mkdir(SDCARD_MOUNT_POINT "/data", 0777);
    mkdir(SDCARD_MOUNT_POINT "/programs", 0777);
    mkdir(SDCARD_MOUNT_POINT "/system", 0777);
    mkdir(SDCARD_MOUNT_POINT "/roms", 0777);

    return ESP_OK;
}

void sdcard_deinit(void)
{
    if (s_mounted) {
        esp_vfs_fat_sdcard_unmount(SDCARD_MOUNT_POINT);
        s_mounted = false;
    }
}

bool sdcard_is_mounted(void) { return s_mounted; }
