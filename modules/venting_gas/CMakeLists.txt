add_executable(module_venting_gas
    main.c
)

target_link_libraries(module_venting_gas
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_i2c
)

pico_add_extra_outputs(module_venting_gas)
pico_enable_stdio_usb(module_venting_gas 1)
pico_enable_stdio_uart(module_venting_gas 0)
