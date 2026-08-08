idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES esp_lcd
                                     esp_timer
                                     esp_driver_gpio
                                     esp_driver_spi
                                     esp_driver_i2c
                                     esp_hw_support
                                     heap
                                     spi_flash
                                     lvgl
                                     app_update
                                     nvs_flash
                                     fatfs
                                     sdmmc
                                     xiaomiao_hal
                                     app_manager
                       INCLUDE_DIRS "")
target_link_libraries(${COMPONENT_LIB} PRIVATE m)