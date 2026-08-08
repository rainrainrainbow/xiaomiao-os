idf_component_register(SRCS "xiaomiao_hal.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_lcd
                                     esp_timer
                                     esp_driver_gpio
                                     esp_driver_spi
                                     esp_driver_i2c
                                     esp_hw_support
                                     heap
                                     spi_flash
                                     lvgl
                                     fatfs
                                     sdmmc)