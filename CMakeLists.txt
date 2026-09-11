cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK (must be before project())
include(pico_sdk_import.cmake)

project(ktane C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Shared library used by all targets
add_subdirectory(shared)

# Individual firmware targets — comment out any you haven't wired yet
add_subdirectory(central_controller)
add_subdirectory(modules/wires)
add_subdirectory(modules/keypad)
add_subdirectory(modules/simon_says)
add_subdirectory(modules/whos_on_first)
add_subdirectory(modules/memory)
add_subdirectory(modules/switches)
add_subdirectory(modules/maze)
add_subdirectory(modules/password)
add_subdirectory(modules/venting_gas)
add_subdirectory(modules/knob)
