# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.21)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(gcbot C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Strip fatFS DBG_PRINTF
add_definitions(-DNDEBUG)

set(SOURCES 
    gcbot.cpp
    st7735red.cpp 
    dtm_reader.cpp
    dtm_writer.cpp
    hw_config.c
    ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/joybus.c
    ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/GamecubeConsole.cpp
    ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/GamecubeController.cpp
)

add_executable(gcbot ${SOURCES})
pico_set_program_name(gcbot "gcbot")
pico_set_program_version(gcbot "1.0")

# Generate PIO header
pico_generate_pio_header(gcbot ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/src/joybus.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(gcbot 1)
pico_enable_stdio_usb(gcbot 0)

add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src build)

target_include_directories(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico BEFORE INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add the standard library to the build
target_link_libraries(gcbot
        pico_stdlib
)

# Add the standard include files to the build
target_include_directories(gcbot PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/joybus-pio/include/
)

# Add any user requested libraries
target_link_libraries(gcbot 
        pico_multicore
        hardware_pio
        hardware_clocks
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

pico_add_extra_outputs(gcbot)
