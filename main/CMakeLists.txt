# XiaoMiao OS Main Component

set(SOURCES
    main.c
    hal/lcd.c
    hal/sdcard.c
    hal/keys.c
    hal/backlight.c
    hal/battery.c
    hal/i2c_bus.c
    hal/gyro.c
    hal/buzzer.c
    ui/theme.c
    ui/fonts.c
    ui/components.c
    ui/page_manager.c
    desktop/desktop.c
    desktop/launcher.c
    desktop/statusbar.c
    desktop/settings.c
    block_editor/editor.c
    block_editor/block_def.c
    block_editor/block_tree.c
    block_editor/codegen.c
    block_editor/renderer.c
    block_editor/input.c
    micropython/mp_engine.c
    micropython/mp_module_xiaomiao.c
    app_runtime/app_manager.c
    app_runtime/app_format.c
    app_runtime/app_sandbox.c
)

set(INCLUDE_DIRS
    .
    hal
    ui
    desktop
    block_editor
    micropython
    app_runtime
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES lvgl fatfs spi sdmmc driver adc esp_adc esp_common
)

# Link MicroPython port if available
if(EXISTS ${CMAKE_SOURCE_DIR}/micropython/port/CMakeLists.txt)
    message(STATUS "MicroPython port found - adding to build")
    add_subdirectory(${CMAKE_SOURCE_DIR}/micropython/port mp_port_build)
    target_link_libraries(${COMPONENT_LIB} PRIVATE mp_port)
endif()
