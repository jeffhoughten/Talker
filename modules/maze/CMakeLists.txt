add_executable(module_maze
    main.c
)

target_link_libraries(module_maze
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    pico_i2c_slave
    hardware_spi
)

pico_add_extra_outputs(module_maze)
pico_enable_stdio_usb(module_maze 1)
pico_enable_stdio_uart(module_maze 0)
