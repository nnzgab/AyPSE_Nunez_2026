idf_component_register(SRCS "test_app_main.c"
                            "../../drivers_hal/test/test_gpio_hal.c"
                            "../../board_support/test/test_led.c"
                        INCLUDE_DIRS "."
                        REQUIRES unity drivers_hal board_support esp_driver_gpio
                        WHOLE_ARCHIVE)
