add_executable(central_controller
    main.c
    game_state.c
    ht16k33.c
    ssd1306.c
    save.c
)

target_link_libraries(central_controller
    ktane_shared
    pico_stdlib
    hardware_gpio
    hardware_i2c
    hardware_spi
    hardware_pio
    hardware_flash
    hardware_sync
)

target_include_directories(central_controller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Generate UF2 for drag-and-drop flashing
pico_add_extra_outputs(central_controller)

# Optional: enable USB serial for debugging
pico_enable_stdio_usb(central_controller 1)
pico_enable_stdio_uart(central_controller 0)
