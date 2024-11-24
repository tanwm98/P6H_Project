#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy3/protocol_analyzer.h"

// Function prototypes
void print_menu(void);
void process_command(char cmd);
void setup_gpio_interrupts(void);

// Tracking variables for real-time display
static uint32_t last_display_time = 0;
static const uint32_t DISPLAY_INTERVAL_MS = 250; // Update display every 250ms

int main() {
    // Initialize stdio and USB
    stdio_init_all();
    sleep_ms(2000); // Give time for serial terminal to connect
    printf("\nPico Protocol Analyzer - UART Monitor Mode\n");
    printf("Configured to monitor UART on GP%d\n", UART_RX_PIN);

    // Initialize protocol analyzer
    protocol_analyzer_init();
    
    // Setup GPIO interrupts for UART monitoring
    setup_gpio_interrupts();

    // Initial menu display
    print_menu();
    
    // Start capture immediately
    start_protocol_capture();

    // Main loop - process commands and display data
    while (true) {
        int c = getchar_timeout_us(0); // Non-blocking character read
        if (c != PICO_ERROR_TIMEOUT) {
            process_command((char)c);
            if (c == '4') break; // Exit command
        }

        // Regular status updates while capturing
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (is_protocol_capturing() && 
            (current_time - last_display_time) >= DISPLAY_INTERVAL_MS) {
            
            ProtocolMetrics metrics = get_protocol_metrics();
            if (metrics.edge_count > 0) {
                printf("\rEdges captured: %lu ", metrics.edge_count);
                
                if (metrics.is_valid) {
                    printf("| Protocol: %s ", get_protocol_name(metrics.detected_protocol));
                    if (metrics.detected_protocol == PROTOCOL_UART) {
                        printf("| Baud: %lu ", metrics.baud_rate);
                        printf("| Error: %.1f%%", metrics.error_margin);
                    }
                }
                
                fflush(stdout); // Ensure output is displayed immediately
            }
            
            last_display_time = current_time;
        }

        tight_loop_contents();
    }

    return 0;
}

void setup_gpio_interrupts(void) {
    // Setup edge detection interrupt for UART RX pin
    gpio_set_irq_enabled_with_callback(UART_RX_PIN, 
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, 
                                     (gpio_irq_callback_t)handle_protocol_edge);
    
    printf("GPIO interrupt enabled on pin %d\n", UART_RX_PIN);
}

void print_menu(void) {
    printf("\n\n=== Protocol Analyzer Menu ===\n");
    printf("1: Start new capture\n");
    printf("2: Stop capture and show detailed analysis\n");
    printf("3: Show current metrics\n");
    printf("4: Exit\n");
    printf("Ready for command...\n");
}

void process_command(char cmd) {
    ProtocolMetrics metrics;
    
    switch (cmd) {
        case '1':
            printf("\nStarting new protocol capture...\n");
            start_protocol_capture();
            break;

        case '2':
            printf("\nStopping capture and analyzing...\n");
            stop_protocol_capture();
            metrics = get_protocol_metrics();
            if (metrics.is_valid) {
                printf("\nFinal Analysis Results:\n");
                printf("Protocol: %s\n", get_protocol_name(metrics.detected_protocol));
                if (metrics.detected_protocol == PROTOCOL_UART) {
                    printf("Baud Rate: %lu\n", metrics.baud_rate);
                    printf("Error Margin: %.1f%%\n", metrics.error_margin);
                    printf("Total Samples: %lu\n", metrics.sample_count);
                }
            } else {
                printf("No valid protocol detected\n");
            }
            print_menu();
            break;

        case '3':
            metrics = get_protocol_metrics();
            printf("\nCurrent Analysis Status:\n");
            printf("Capturing: %s\n", metrics.is_capturing ? "Yes" : "No");
            printf("Edge Count: %lu\n", metrics.edge_count);
            if (metrics.is_valid) {
                printf("Protocol: %s\n", get_protocol_name(metrics.detected_protocol));
                if (metrics.detected_protocol == PROTOCOL_UART) {
                    printf("Baud Rate: %lu\n", metrics.baud_rate);
                    printf("Error Margin: %.1f%%\n", metrics.error_margin);
                }
            } else {
                printf("No valid protocol detected yet\n");
            }
            print_menu();
            break;

        case '4':
            printf("\nExiting program...\n");
            break;

        case '\n':
        case '\r':
            // Ignore newline/carriage return
            break;

        default:
            if (cmd >= ' ') { // Only show error for printable chars
                printf("Invalid command: '%c'\n", cmd);
                print_menu();
            }
            break;
    }
}