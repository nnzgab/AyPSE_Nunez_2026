idf_component_register(SRCS "test_app_main.c"
                            "../../drivers_hal/test/test_gpio_hal.c"
                            "../../drivers_hal/test/test_uart_hal.c"
                            "../../board_support/test/test_led.c"
                            "../../board_support/test/test_panic_button.c"
                            "../../board_support/test/test_cellular.c"
                            "../../middleware/test/test_status_indicator.c"
                            "../../middleware/test/test_panic_handler.c"
                            "../../middleware/test/test_cellular_net.c"
                        INCLUDE_DIRS "."
                        REQUIRES unity drivers_hal board_support middleware esp_driver_gpio esp_timer
                        WHOLE_ARCHIVE)
