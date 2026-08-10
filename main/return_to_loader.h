// ================ return_to_loader.h - 返回原厂 Loader ================
// 按住 BACK 键 + 短按 RESET（或断电重启）可回到 Loader 选 ROM
// 实现: 在 NVS 写入 "return_to_loader" 标记, 后由 bootloader 检测
// 简易实现: 直接 esp_restart() 触发 boot 阶段的固件选择

#ifndef __RETURN_TO_LOADER_H__
#define __RETURN_TO_LOADER_H__

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

static inline void return_to_loader_setup(void)
{
    // 写入标记，下次上电由 bootloader 跳过当前 app
    nvs_handle_t h;
    if (nvs_open("loader", NVS_READWRITE, &h) == ESP_OK) {
        uint8_t flag = 1;
        nvs_set_blob(h, "return", &flag, 1);
        nvs_commit(h);
        nvs_close(h);
    }
}

static inline void return_to_loader_set_app_desc(const char *name, const char *desc)
{
    // 写入 app 描述, Loader 选 ROM 界面显示
    nvs_handle_t h;
    if (nvs_open("loader", NVS_READWRITE, &h) == ESP_OK) {
        if (name) nvs_set_str(h, "app_name", name);
        if (desc) nvs_set_str(h, "app_desc", desc);
        nvs_commit(h);
        nvs_close(h);
    }
}

static inline void return_to_loader_now(void)
{
    return_to_loader_setup();
    esp_restart();
}

#endif // __RETURN_TO_LOADER_H__