idf_component_register(SRCS "st7701_probe.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd esp_driver_ledc esp_hw_support)
