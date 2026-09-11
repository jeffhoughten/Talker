add_executable(module_whos_on_first
    main.c
)

target_link_libraries(module_whos_on_first
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_i2c
)

pico_add_extra_outputs(module_whos_on_first)
pico_enable_stdio_usb(module_whos_on_first 1)
pico_enable_stdio_uart(module_whos_on_first 0)
