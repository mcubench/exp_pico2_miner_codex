cmake_minimum_required(VERSION 3.13)

# These cache defaults remain overrideable by tools/build.
set(PICO_BOARD pico2 CACHE STRING "Target board")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "RP2350 CPU architecture")

include(pico_sdk_import.cmake)

project(pico2_agent C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico2_agent
    src/main.c
)

target_link_libraries(pico2_agent PRIVATE
    pico_stdlib
)

target_compile_options(pico2_agent PRIVATE
    -Wall
    -Wextra
    -Werror
)

# USB CDC supplies both logs and picotool's automatic reset-to-BOOTSEL path.
pico_enable_stdio_usb(pico2_agent 1)
pico_enable_stdio_uart(pico2_agent 0)

target_compile_definitions(pico2_agent PRIVATE
    PICO_ENTER_USB_BOOT_ON_EXIT=1
)

pico_add_extra_outputs(pico2_agent)
