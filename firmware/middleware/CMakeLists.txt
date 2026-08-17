idf_build_get_property(target IDF_TARGET)

# Always compiled source files
set(srcs "src/event_frame.c"
    )

# Always included headers
set(includes "inc")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       PRIV_REQUIRES board_support drivers_hal)