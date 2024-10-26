#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "uart.h"


void uart_init_all() {
    uart_init(UART_ID, MAX_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
}

void uart_send_string(const char* str) {
    while (*str) {
        uart_putc(UART_ID, *str++);
    }
}

char uart_receive_char() {
    return uart_getc(UART_ID);
}

