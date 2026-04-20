cmake_minimum_required(VERSION 3.13)
include(FetchContent)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

project(secondary C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(secondary src/main.c src/display.c)

pico_set_program_name(secondary "secondary")
pico_set_program_version(secondary "0.1")

# LVGL needs an lv_conf.h; point it to the project-local config header.
set(LV_CONF_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
    CACHE STRING "" FORCE)

# Fetch LVGL from GitHub
FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG v9.2.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(lvgl)

# The target "MyFirmware" depends on LVGL
target_link_libraries(secondary PRIVATE lvgl::lvgl)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(secondary 0)
pico_enable_stdio_usb(secondary 0)

# Add the standard library to the build
target_link_libraries(secondary PRIVATE
    pico_stdlib)

# Add the standard include files to the build
target_include_directories(secondary PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(secondary PRIVATE
        hardware_spi
        hardware_i2c
        )

pico_add_extra_outputs(secondary)

