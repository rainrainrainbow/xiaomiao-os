#pragma once
/*
 * return_to_loader.h
 * 
 * 确保 Reset / 断电重启后返回 Loader 而非重复进入当前 ROM。
 * 原理：通过 RTC_CNTL_STORE0_REG 写入魔数，bootloader 检测到后
 * 会 set_boot(factory) 回到 Loader。
 *
 * 用法：在 app_main() 第一行调用 return_to_loader_setup()
 */

#include "esp_err.h"
#include "esp_system.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

#define LOADER_MAGIC_A 0x4C4F4144  // "LOAD"
#define LOADER_MAGIC_B 0x45524D49  // "ERMI"

/*
 * 告知 bootloader 下次启动回到 factory (Loader)
 * 在 app_main() 最开始调用即可
 */
static inline void return_to_loader_setup(void)
{
    /* 写魔数到 RTC 备用寄存器 */
    REG_WRITE(RTC_CNTL_STORE0_REG, LOADER_MAGIC_A);
    REG_WRITE(RTC_CNTL_STORE1_REG, LOADER_MAGIC_B);
}

/*
 * 清除魔数，允许正常启动 ota_0
 * 在确认进入正常流程后调用
 */
static inline void return_to_loader_clear(void)
{
    REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    REG_WRITE(RTC_CNTL_STORE1_REG, 0);
}

/*
 * 检查是否从 Loader 跳转而来
 */
static inline bool return_to_loader_check(void)
{
    uint32_t a = REG_READ(RTC_CNTL_STORE0_REG);
    uint32_t b = REG_READ(RTC_CNTL_STORE1_REG);
    return (a == LOADER_MAGIC_A && b == LOADER_MAGIC_B);
}
