# ================ main/CMakeLists ================

# LVGL 9.5 esp.cmake 第 74 行硬编码 -DLV_CONF_INCLUDE_SIMPLE,
# 它会让 LVGL 内部 .c 文件 #include "lv_conf.h" 时只在 esp.cmake
# 暴露的 INCLUDE_DIRS (LVGL_ROOT_DIR、LVGL_ROOT_DIR/src、LVGL_ROOT_DIR/../)
# 里搜索. main/ 目录不在搜索范围内, 所以我们必须把 lvgl_conf.h
# 复制到 ${LVGL_ROOT_DIR}/lv_conf.h, 让 esp.cmake 简单模式下能找到.
#
# LVGL_ROOT_DIR 是 LVGL 组件顶层 CMakeLists.txt 在 ESP_PLATFORM 块
# 之前 set 出来的全局变量, 在这里可以直接使用.
if(DEFINED LVGL_ROOT_DIR)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/lvgl_conf.h"
        "${LVGL_ROOT_DIR}/lv_conf.h"
        COPYONLY
    )
    message(STATUS "[xiaomiao-os] 已把 lvgl_conf.h 复制到 ${LVGL_ROOT_DIR}/lv_conf.h")
else()
    message(FATAL_ERROR "[xiaomiao-os] LVGL_ROOT_DIR 未定义, LVGL 组件未正确声明")
endif()

idf_component_register(
    SRCS
        "main.c"
        "hal/lcd_st7735.c"
        "hal/keys.c"
        "hal/sdcard.c"
        "hal/buzzer.c"
        "hal/battery.c"
        "hal/mpu6050.c"
        "hal/led_motor.c"
        "ui/theme.c"
        "ui/input.c"
        "ui/toast.c"
        "ui/canvas.c"
        "core/settings.c"
        "core/app_manager.c"
        "core/app_loader.c"
        "mpy/mp_runtime.c"
        "mpy/mp_bindings.c"
        "desktop/builtins.c"
    INCLUDE_DIRS
        "."
        "hal"
        "ui"
        "core"
        "mpy"
        "desktop"
    PRIV_REQUIRES
        driver
        nvs_flash
        esp_lcd
        esp_adc
        esp_timer
        sdmmc
        fatfs
        lvgl
        esp_wifi
        # micropython  (开启 CONFIG_ENABLE_MICROPYTHON 后由 idf_component 拉取)
)