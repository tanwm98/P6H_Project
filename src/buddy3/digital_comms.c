#include "digital_comms.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 8
#define UART_RX_PIN 9

void digital_comms_init() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

void process_uart_command() {
    if (uart_is_readable(UART_ID)) {
        char command = uart_getc(UART_ID);
        // Process command (implementation details omitted for brevity)
    }
}
