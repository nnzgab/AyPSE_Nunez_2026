idf_build_get_property(target IDF_TARGET)

# Always compiled source files
set(srcs "src/gpio_hal.c" "src/uart_hal.c")

# Always included headers
set(includes "inc")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "inc"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_uart)