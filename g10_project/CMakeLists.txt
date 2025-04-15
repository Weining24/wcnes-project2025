cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(backscatter C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

# Create the executable with the name "backscatter"
add_executable(backscatter)

# Link only the libraries needed for backscatter: pico_stdlib for Pico functions and hardware_pio for PIO operations.
target_link_libraries(backscatter PRIVATE pico_stdlib hardware_pio)
pico_add_extra_outputs(backscatter)

# Enable USB stdio (UART is disabled here but can be enabled if desired)
pico_enable_stdio_usb(backscatter 1)
pico_enable_stdio_uart(backscatter 0)

# Add the source files. Only main.c, packet_generation.c, and backscatter.c from your external library folder are needed.
target_sources(backscatter PRIVATE 
        main.c
        ../project_pico_libs/packet_generation.c
        ../project_pico_libs/backscatter.c
)
include_directories(../project_pico_libs)

# Optional compile options
add_compile_options(-Wall
        -Wno-format          # Suppress warnings about type differences
        -Wno-unused-function # Some helper functions might be unused
        -Wno-maybe-uninitialized
)
