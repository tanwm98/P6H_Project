add_executable(station2
    buddy1/sd_card.c
    buddy1/hw_config.c
    station2.c
    buddy2/adc.c
    buddy2/digital.c
    buddy2/pwm.c
    buddy3/protocol_analyzer.c
    buddy3/uart.c
    buddy3/i2c.c
    buddy3/spi.c
    buddy4/swd.c
    buddy5/ntp.c
    buddy5/wifi_dashboard.c

    ${PICO_LWIP_CONTRIB_PATH}/apps/ping/ping.c
)

target_include_directories(station2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
    ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
    ${PICO_LWIP_CONTRIB_PATH}/apps/ping
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(station2
    pico_stdlib
    hardware_uart 
    hardware_pwm
    hardware_adc
    hardware_spi
    hardware_i2c
    hardware_gpio
    FatFs_SPI
    pico_cyw43_arch_lwip_poll   
    pico_lwip_iperf                # lwIP library
    pico_lwip_sntp
)

pico_enable_stdio_usb(station2 1)
pico_enable_stdio_uart(station2 1)

pico_add_extra_outputs(station2)
