idf_build_get_property(target IDF_TARGET)

# Always compiled source files
set(srcs    "src/led.c" 
            "src/panic_button.c" 
            "src/cellular_modem.c"
            "src/board_clock.c"
    )

# Always included headers
set(includes "inc")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       PRIV_REQUIRES drivers_hal)