add_executable(module_memory
    main.c
)

target_link_libraries(module_memory
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_spi
)

pico_add_extra_outputs(module_memory)
pico_enable_stdio_usb(module_memory 1)
pico_enable_stdio_uart(module_memory 0)
