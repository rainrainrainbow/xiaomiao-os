# ================ main/CMakeLists ================

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

# LVGL 9.5 esp.cmake 第 74 行硬编码 -DLV_CONF_INCLUDE_SIMPLE,
# 它会让 LVGL 内部 .c 文件 #include "lv_conf.h" 时只在 esp.cmake
# 暴露的 INCLUDE_DIRS (LVGL_ROOT_DIR、LVGL_ROOT_DIR/src、LVGL_ROOT_DIR/../)
# 里搜索. main/ 目录不在搜索范围内, 所以我们必须把 lvgl_conf.h
# 复制到 lvgl 组件根目录下的 lv_conf.h, 让 esp.cmake 简单模式下能找到.
#
# 在 idf_component_register 之后, IDF 已解析完所有依赖, 可以用
# idf_component_get_property 拿到 lvgl 组件的实际目录.
# 注意: component manager 拉取的组件名是 lvgl__lvgl (vendor__package).
# 如果找不到, 回退到 managed_components/lvgl__lvgl 目录.
#
# 注意: configure_file 在 configure 阶段执行, 但 managed_components
# 目录此时可能不存在. 改用 add_custom_command 在 build 阶段复制,
# 确保 LVGL 的 .c 文件编译时 lv_conf.h 已就位.
get_filename_component(proj_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(lvgl_dir "${proj_root}/managed_components/lvgl__lvgl")
if(NOT EXISTS "${lvgl_dir}")
    # 尝试用 idf_component_get_property 找
    idf_component_get_property(lvgl_dir lvgl__lvgl COMPONENT_DIR)
    if(NOT lvgl_dir)
        idf_component_get_property(lvgl_dir lvgl COMPONENT_DIR)
    endif()
    if(NOT lvgl_dir)
        message(FATAL_ERROR "[xiaomiao-os] 无法定位 lvgl 组件目录 (${lvgl_dir} 不存在)")
    endif()
endif()
# 用 add_custom_command 在 build 阶段复制 (确保目标目录已存在)
add_custom_command(TARGET ${COMPONENT_LIB} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/lvgl_conf.h"
        "${lvgl_dir}/lv_conf.h"
    COMMENT "[xiaomiao-os] 复制 lvgl_conf.h -> ${lvgl_dir}/lv_conf.h"
)
message(STATUS "[xiaomiao-os] 注册了 lvgl_conf.h 复制到 ${lvgl_dir}/lv_conf.h")