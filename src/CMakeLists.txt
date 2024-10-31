add_executable(${NAME}
    main.c
    buddy3/signal_generator.c
    buddy3/protocol_analyzer.c
    buddy1/sd_card.c
    buddy1/hw_config.c
    buddy2/signal_analyzer.c
)


target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}/../lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src/include
    ${CMAKE_CURRENT_LIST_DIR}/../lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/ff15/source  # Add FatFS headers
    ${CMAKE_CURRENT_LIST_DIR}/../lib/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src    # Add SD card driver headers
)
add_compile_definitions(USE_PRINTF)
target_compile_definitions(${NAME} PRIVATE
    NO_SYS=0
    LWIP_SOCKET=1
)

target_link_libraries(${NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
    hardware_uart 
    hardware_pwm
    hardware_adc
    hardware_spi
    hardware_i2c
    hardware_clocks
    hardware_gpio
    hardware_dma
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)

pico_add_extra_outputs(${NAME})