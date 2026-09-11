add_executable(module_keypad
    main.c
)

target_link_libraries(module_keypad
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_i2c
)

pico_add_extra_outputs(module_keypad)
pico_enable_stdio_usb(module_keypad 1)
pico_enable_stdio_uart(module_keypad 0)
