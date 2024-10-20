#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_TX_PIN 0
#define UART_RX_PIN 1

void uart_init_all() {
    uart_init(UART_ID, BAUD_RATE);
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

int main() {
    stdio_init_all();
    uart_init_all();

    while (1) {
        uart_send_string("Hello from Pico W!\r\n");
        sleep_ms(1000);

        if (uart_is_readable(UART_ID)) {
            char c = uart_receive_char();
            printf("Received: %c\n", c);
        }
    }

    return 0;
}