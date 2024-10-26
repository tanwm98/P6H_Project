#ifndef UART_H
#define UART_H

#define UART_ID uart0
#define MIN_BAUD_RATE 4800
#define MAX_BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#include "hardware/uart.h"

void uart_init_all();
void uart_send_string(const char* str);
char uart_receive_char();
#endif
