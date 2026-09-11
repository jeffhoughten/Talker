add_executable(module_password
    main.c
)

target_link_libraries(module_password
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_i2c
)

pico_add_extra_outputs(module_password)
pico_enable_stdio_usb(module_password 1)
pico_enable_stdio_uart(module_password 0)
