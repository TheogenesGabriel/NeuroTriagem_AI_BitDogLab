# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico_w)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(smile_display C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(smile_display
    main.c
    ssd1306.c
)

target_include_directories(smile_display PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# lwipopts.h lives in this folder
target_include_directories(smile_display PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(smile_display
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_i2c
)

pico_enable_stdio_usb(smile_display 1)
pico_enable_stdio_uart(smile_display 0)

pico_add_extra_outputs(smile_display)
