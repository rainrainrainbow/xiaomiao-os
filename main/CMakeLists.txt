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

# 让 LVGL 9 使用我们的 lvgl_conf.h (160x128 极小屏配置)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LV_CONF_PATH="${CMAKE_CURRENT_LIST_DIR}/lvgl_conf.h"
)