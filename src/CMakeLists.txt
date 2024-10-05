add_executable(${NAME}
    main.c
    buddy3/digital_comms.c
    buddy3/signal_generator.c
    buddy3/automate.c     
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

target_link_libraries(${NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    FreeRTOS-Kernel-Heap4
    hardware_uart 
    hardware_pwm
)

pico_enable_stdio_usb(${NAME} 1)    # Disable USB printf
pico_enable_stdio_uart(${NAME} 0)   # Enable UART printf

pico_add_extra_outputs(${NAME})