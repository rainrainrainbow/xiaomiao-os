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
                       INCLUDE_DIRS "ui")
target_link_libraries(${COMPONENT_LIB} PRIVATE m)
