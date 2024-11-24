add_executable(${NAME}
    main.c
    buddy1/sd_card.c
    buddy1/hw_config.c
    buddy2/digital.c
    buddy3/protocol_analyzer.c
    buddy5/wifi.c
)

target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)


target_link_libraries(${NAME}
    pico_stdlib
    hardware_uart 
    hardware_pwm
    hardware_adc
    hardware_spi
    hardware_i2c
    hardware_gpio
    FatFs_SPI
    pico_cyw43_arch_lwip_poll  
    pico_lwip_sntp  
)

pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)

pico_add_extra_outputs(${NAME})
