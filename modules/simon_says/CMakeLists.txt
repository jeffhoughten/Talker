add_executable(module_simon_says
    main.c
)

target_link_libraries(module_simon_says
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_pwm
)

pico_add_extra_outputs(module_simon_says)
pico_enable_stdio_usb(module_simon_says 1)
pico_enable_stdio_uart(module_simon_says 0)
