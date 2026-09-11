add_executable(module_knob
    main.c
)

target_link_libraries(module_knob
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_gpio
)

pico_add_extra_outputs(module_knob)
pico_enable_stdio_usb(module_knob 1)
pico_enable_stdio_uart(module_knob 0)
