add_executable(${NAME}
    main.c
)


target_include_directories(${NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_definitions(${NAME} PRIVATE
    NO_SYS=0
    LWIP_SOCKET=1
)

target_link_libraries(test_pico
    pico_stdlib
    hardware_uart
    pico_cyw43_arch_none    # For Pico W LED support
    hardware_pwm
    hardware_i2c
    hardware_spi
)

pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 1)

pico_add_extra_outputs(${NAME})