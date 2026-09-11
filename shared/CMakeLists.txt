add_library(ktane_shared STATIC
    bus_protocol.c
    module_base.c
)

target_include_directories(ktane_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    hardware_spi
    pico_i2c_slave      # I2C slave mode for the module Picos (SDK >= 1.3.0)
)
