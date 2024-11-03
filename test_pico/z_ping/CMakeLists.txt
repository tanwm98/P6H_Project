cmake_minimum_required(VERSION 3.12)

project(z_ping)

# Initialize the Pico SDK
pico_sdk_init()

if (EXISTS ${PICO_LWIP_CONTRIB_PATH}/apps/ping/ping.c)

    add_executable(z_ping
    z_ping.c
            ${PICO_LWIP_CONTRIB_PATH}/apps/ping/ping.c
                dhcpserver/dhcpserver.c
                dnsserver/dnsserver.c
            )

    # Set the WiFi credentials for compilation
    target_compile_definitions(z_ping PRIVATE
            WIFI_SSID=\"${WIFI_SSID}\"
            WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
            )

    # Include directories for FreeRTOS and lwIP
    target_include_directories(z_ping PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/.. # for our common FreeRTOSConfig
            ${CMAKE_CURRENT_LIST_DIR}/../.. # for our common lwipopts
            ${PICO_LWIP_CONTRIB_PATH}/apps/ping
            ${CMAKE_CURRENT_LIST_DIR}/../include
            )

    # Link necessary libraries
    target_link_libraries(z_ping 
            hardware_adc                   # To use ADC hardware
            pico_cyw43_arch_lwip_threadsafe_background # Thread-safe Wi-Fi
            pico_stdlib                    # Standard library for Pico SDK
            pico_lwip_iperf                # lwIP library
            hardware_pwm
            )

    # Enable USB output for printf()
    pico_enable_stdio_usb(z_ping 1)   # Enable USB stdio
    pico_enable_stdio_uart(z_ping 0)  # Disable UART stdio

    # Add extra outputs for flashing and other files
    pico_add_extra_outputs(z_ping)
endif()
