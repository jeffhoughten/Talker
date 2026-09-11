# This file is copied from the Pico SDK itself.
# It lets you build without installing the SDK globally by setting
# PICO_SDK_PATH in your environment or on the cmake command line.

if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR
        "PICO_SDK_PATH is not set.\n"
        "Set it in your environment: set PICO_SDK_PATH=C:/path/to/pico-sdk\n"
        "Or pass it to cmake: cmake -DPICO_SDK_PATH=C:/path/to/pico-sdk .."
    )
endif ()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

if (NOT EXISTS "${PICO_SDK_PATH}/pico_sdk_version.cmake")
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' does not appear to contain the Pico SDK")
endif ()

include(${PICO_SDK_PATH}/pico_sdk_version.cmake)
message("Found Pico SDK version ${PICO_SDK_VERSION_STRING} in '${PICO_SDK_PATH}'")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
