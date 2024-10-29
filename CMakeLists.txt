cmake_minimum_required(VERSION 3.16)

set(NAME P6H_Project)
set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PICO_SDK_PATH "C:/Program Files/Raspberry Pi/Pico SDK v1.5.1/pico-sdk") 
#change this accordingly to where you installed it
#if it doesnt work, F1 > Clean CMAKE cache and reconfigure then go CMake and click the triple dots and "Clean Reconfigure all projects"

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake) ## follow fauzi lab to get this

add_subdirectory(src)
add_subdirectory(build/lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)