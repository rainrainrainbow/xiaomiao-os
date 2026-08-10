// ================ sdcard.c - TF 卡 SDSPI 驱动 ================

#include "sdcard.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_heap_caps.h"

static const char *TAG = "sdcard";
static bool s_mounted = false;

esp_err_t sdcard_init(void)
{
    if (s_mounted) return ESP_OK;

    // SPI2 已被 LCD 占用, 直接挂设备
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = 20000;  // 20 MHz 安全
    host.flags = SDMMC_HOST_FLAG_SPI;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false,
    };

    sdmmc_card_t *card = NULL;
    esp_err_t err = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config,
                                             &mount_config, &card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD mount fail: %s", esp_err_to_name(err));
        return err;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "SD mounted at %s", SD_MOUNT_POINT);
    return ESP_OK;
}

void sdcard_deinit(void)
{
    if (!s_mounted) return;
    esp_vfs_fat_sdmmc_unmount();
    s_mounted = false;
}

bool sdcard_is_mounted(void) { return s_mounted; }

uint64_t sdcard_total_bytes(void)
{
    // 简版: 需保留 card 句柄, 这里仅返回 0 占位
    return 0;
}

uint64_t sdcard_free_bytes(void)
{
    FATFS *fs = NULL;
    DWORD fre_clust;
    if (f_getfree(SD_MOUNT_POINT, &fre_clust, &fs) != FR_OK) return 0;
    uint64_t total = (uint64_t)(fs->n_fatent - 2) * fs->csize;
    uint64_t free  = (uint64_t)fre_clust * fs->csize;
    return free * 512;
}