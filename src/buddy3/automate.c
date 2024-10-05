#include "automate.h"
#include "signal_generator.h"
#include "digital_comms.h"
#include "hardware/uart.h"

#define UART_ID uart1

void automation_init() {
    // Any necessary initialization for automation
}

void run_automated_sequence() {
    generate_signal(500, 500);
    uart_puts(UART_ID, "Signal complete\n");
}