add_executable(main2
    main2.c
    buddy3/signal_generator.c
    buddy3/protocol_analyzer.c
    buddy1/sd_card.c
    buddy1/hw_config.c
    buddy2/signal_analyzer.c
    buddy2/adc.c
    buddy2/pwm.c
)


target_include_directories(main2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_definitions(main2 PRIVATE
    NO_SYS=0
    LWIP_SOCKET=1
)

target_link_libraries(main2
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
    hardware_uart 
    hardware_pwm
    hardware_adc
    hardware_spi
    hardware_i2c
    hardware_gpio
    FatFs_SPI
)

pico_enable_stdio_usb(main2 1)
pico_enable_stdio_uart(main2 0)

pico_add_extra_outputs(main2)
