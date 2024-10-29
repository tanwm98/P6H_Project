#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "buddy3/signal_generator.h"

const sequence_data_t test_sequence = {
    .pulses = {
        {0,      1},      // Start HIGH
        {100000, 0},    // Go LOW after 100ms
        {200000, 1},    // HIGH after 200ms
        {300000, 0},    // LOW after 300ms
        {400000, 1},    // HIGH after 400ms
        {500000, 0},    // LOW after 500ms
        {600000, 1},    // HIGH after 600ms
        {700000, 0},    // LOW after 700ms
        {800000, 1},    // HIGH after 800ms
        {900000, 0}     // LOW after 900ms
    },
    .count = 10,
    .sequence_verified = false
};

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\nSignal Generator & UART Analyzer - Week 10\n");
    signal_generator_init();
    
    while (1) {
        printf("\nTest Menu:\n");
        printf("1. Generate pulse sequence (GP3)\n");
        printf("2. Analyze UART signal (GP4)\n");
        printf("3. Exit\n");
        printf("Choice: ");
        
        char choice = getchar();
        printf("\n");
        
        switch (choice) {
            case '1': {
                printf("Generating pulse sequence...\n");
                signal_status_t status = reproduce_sequence(&test_sequence);
                
                switch (status) {
                    case SG_SUCCESS:
                        printf("✓ Sequence completed successfully\n");
                        break;
                    default:
                        printf("✗ Sequence generation failed\n");
                        break;
                }
                break;
            }
            case '2': {
                printf("Analyzing UART signal on GP4...\n");
                printf("Ensure test device is connected and transmitting\n");
                uart_result_t result = analyze_uart_signal(1000);
                display_uart_results(&result);
                break;
            }
            case '3':{
                printf("Exiting...\n");
                return 0;
            }
            default:
                printf("Invalid choice\n");
                break;
        }
        sleep_ms(1000);
    }
    return 0;
}