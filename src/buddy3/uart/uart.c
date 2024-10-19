// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "hardware/uart.h"
// #include "hardware/gpio.h"

// #define UART_ID uart0
// #define UART_TX_PIN 0
// #define UART_RX_PIN 1

// #define MAX_MESSAGE_LENGTH 256
// #define NUM_BAUD_RATES 7

// const uint32_t baud_rates[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800};

// // Function to detect baud rate
// uint32_t detect_baud_rate() {
//     printf("Detecting baud rate...\n");
//     for (int i = 0; i < NUM_BAUD_RATES; i++) {
//         uart_init(UART_ID, baud_rates[i]);
//         gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
//         gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

//         // Send a test character
//         uart_putc(UART_ID, 'U');

//         // Wait for a short time
//         sleep_ms(100);

//         // Check if we received the test character back
//         if (uart_is_readable(UART_ID)) {
//             char c = uart_getc(UART_ID);
//             if (c == 'U') {
//                 printf("Detected baud rate: %u\n", baud_rates[i]);
//                 return baud_rates[i];
//             }
//         }
//     }
//     printf("Failed to detect baud rate. Using default 115200.\n");
//     return 115200;
// }

// bool uart_initialize(uint32_t baud_rate, uart_parity_t parity) {
//     uart_init(UART_ID, baud_rate);
//     gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
//     gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
// }

// uart_parity_t detect_uart_parity(uint32_t baud_rate) {
//     const uart_parity_t parities[] = {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD};
//     const char *test_message = "PARITY_TEST";

//     for (int i = 0; i < 3; i++) {
//         uart_init(UART_ID, baud_rate);
//         uart_set_format(UART_ID, 8, 1, parities[i]);

//         // Send test message
//         uart_puts(UART_ID, test_message);

//         // Try to receive the message back
//         char received[20] = {0};
//         if (uart_receive(received, sizeof(received)) && strcmp(received, test_message) == 0) {
//             printf("Detected parity: %s\n");
//             if (parities[i] == UART_PARITY_NONE) {
//                 parities[i] == "None";
//             } else if (parities[i] == UART_PARITY_EVEN) {
//                 parities[i] == "Even";
//             } else {
//                 parities[i] == "Odd";
//             }
//             return parities[i];
//         }
//     }
//     printf("Failed to detect parity. Defaulting to None.\n");
//     return UART_PARITY_NONE;
// }


// // Receive UART data with error checking
// bool uart_receive(char *buffer, size_t buffer_size) {
//     int index = 0;
//     absolute_time_t timeout = make_timeout_time_ms(100); // 100ms timeout

//     while (index < buffer_size - 1) {
//         if (uart_is_readable(UART_ID)) {
//             uint32_t uart_error = uart_get_hw(UART_ID)->rsr;
//             if (uart_error) {
//                 // Handle UART errors (parity error, framing error, break error, overrun error)
//                 if (uart_error & UART_UARTRIS_PERIS_BITS) {
//                     printf("UART Parity Error detected\n");
//                 }
//                 if (uart_error & UART_UARTRIS_FERIS_BITS) {
//                     printf("UART Framing Error detected\n");
//                 }
//                 if (uart_error & UART_UARTRIS_BERIS_BITS) {
//                     printf("UART Break Error detected\n");
//                 }
//                 if (uart_error & UART_UARTRIS_OERIS_BITS) {
//                     printf("UART Overrun Error detected\n");
//                 }
//                 // Clear the error flags
//                 uart_get_hw(UART_ID)->rsr = uart_error;
//                 return false;
//             }

//             char c = uart_getc(UART_ID);
//             buffer[index++] = c;

//             if (c == '\n' || c == '\r') {
//                 break;
//             }
//         } else if (time_reached(timeout)) {
//             break;
//         }
//     }

//     buffer[index] = '\0'; // Null-terminate the string
//     return index > 0; // Return true if we received any data
// }

// // Transmit UART data
// void uart_transmit(const char *message) {
//     uart_puts(UART_ID, message);
// }

// // Process received message
// void process_message(const char *message) {
//     printf("Received: %s", message);
    
//     char response[MAX_MESSAGE_LENGTH];
//     snprintf(response, sizeof(response), "Echo: %s", message);
//     uart_transmit(response);
// }

// void uart_communication_loop() {
//     uint32_t detected_baud_rate = detect_baud_rate();
//     uart_parity_t detected_parity = detect_uart_parity(detected_baud_rate);
//     if (!uart_initialize(detected_baud_rate, UART_PARITY_EVEN)) {
//         return;
//     }

//     char message[MAX_MESSAGE_LENGTH];
//     while (1) {
//         if (uart_receive(message, sizeof(message))) {
//             if (strcmp(message, "exit\n") == 0 || strcmp(message, "exit\r\n") == 0) {
//                 printf("Exiting UART communication.\n");
//                 break;
//             }
//             process_message(message);
//         }
//     }
// }