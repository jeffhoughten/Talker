add_executable(module_switches
    main.c
)

target_link_libraries(module_switches
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_gpio
)

pico_add_extra_outputs(module_switches)
pico_enable_stdio_usb(module_switches 1)
pico_enable_stdio_uart(module_switches 0)
