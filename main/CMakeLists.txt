idf_component_register(SRCS "main.c"
                            "app_manager.c"
                            "app_store.c"
                            "power.c"
                            "micropython_runtime.c"
                            "ui/ui_statusbar.c"
                            "ui/ui_desktop.c"
                            "ui/ui_applist.c"
                            "ui/ui_settings.c"
                            "ui/ui_apprun.c"
                            "ui/ui_appstore.c"
                            "ui/ui_main.c"
PRIV_REQUIRES esp_lcd
                                      esp_timer
                                      esp_driver_gpio
                                      esp_driver_spi
                                      esp_driver_ledc
                                      esp_driver_i2c
                                      esp_hw_support
                                      heap
                                      spi_flash
                                      lvgl
                                      app_update
                                      fatfs
                                      spiffs
                                      esp_adc
                                      micropython
                                      esp_wifi
                       INCLUDE_DIRS "ui"
                                     ".."
                                     "../components/micropython"
                                     "../components/micropython/micropython"
                                     "../components/micropython/micropython/py"
"../components/micropython/micropython/ports/esp32"
                                      "../components/micropython/micropython/ports/esp32/boards/ESP32_GENERIC"
                                      "../components/micropython/micropython/py/genhdr"
                                      "../components/micropython/modules/xiaomiao")
target_compile_options(${COMPONENT_LIB} PRIVATE
    -DMICROPY_PY_SYS=1
    -DMICROPY_PY_SYS_PATH=1
)
target_link_libraries(${COMPONENT_LIB} PRIVATE m)
