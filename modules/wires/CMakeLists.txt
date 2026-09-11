add_executable(module_wires
    main.c
)

target_link_libraries(module_wires
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_adc
)

pico_add_extra_outputs(module_wires)
pico_enable_stdio_usb(module_wires 1)
pico_enable_stdio_uart(module_wires 0)
