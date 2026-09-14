cmake_minimum_required(VERSION 3.13)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# The extension configures its regular ARM build through this helper. Codex's
# dual-build wrapper supplies explicit paths and skips the single-toolchain helper.
if (EXISTS ${picoVscode} AND NOT PICO_AGENT_DUAL_BUILD)
    include(${picoVscode})
endif()
# ====================================================================================

# These cache defaults remain overrideable by tools/build. SYSINFO.PACKAGE_SEL
# on the connected device identifies its package as QFN-60 / RP2350A.
set(PICO_BOARD pico2 CACHE STRING "Target board")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "RP2350 CPU architecture")
set(MINER_SYS_CLOCK_KHZ 150000 CACHE STRING "Miner system clock in kHz")

include(pico_sdk_import.cmake)

project(pico2_agent C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(ORACLE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(ORACLE_INCLUDE "${ORACLE_INCLUDE_DIR}/oracle_vectors.inc")
add_custom_command(
    OUTPUT "${ORACLE_INCLUDE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ORACLE_INCLUDE_DIR}"
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_LIST_DIR}/tools/generate_oracle_vectors.py"
            "${ORACLE_INCLUDE}"
    DEPENDS "${CMAKE_CURRENT_LIST_DIR}/tools/generate_oracle_vectors.py"
    VERBATIM
)
add_custom_target(generate_oracle_vectors DEPENDS "${ORACLE_INCLUDE}")

add_executable(pico2_agent
    src/main.c
)
add_dependencies(pico2_agent generate_oracle_vectors)
target_include_directories(pico2_agent PRIVATE "${ORACLE_INCLUDE_DIR}")

target_link_libraries(pico2_agent PRIVATE
    hardware_dma
    pico_stdlib
    pico_sha256
)

target_compile_options(pico2_agent PRIVATE
    -O3
    -Wall
    -Wextra
    -Werror
)

# USB CDC supplies both logs and picotool's automatic reset-to-BOOTSEL path.
pico_enable_stdio_usb(pico2_agent 1)
pico_enable_stdio_uart(pico2_agent 0)

target_compile_definitions(pico2_agent PRIVATE
    PICO_ENTER_USB_BOOT_ON_EXIT=1
    MINER_SYS_CLOCK_KHZ=${MINER_SYS_CLOCK_KHZ}
)

message(STATUS "Miner system clock: ${MINER_SYS_CLOCK_KHZ} kHz")

pico_add_extra_outputs(pico2_agent)
