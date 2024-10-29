#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "buddy3/signal_generator.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\nSignal Generator & UART Analyzer - Week 10\n");
    signal_generator_init();
    
    while (1) {
        printf("\nTest Menu:\n");
        printf("1. Read stored sequence\n"); //simulate SD Card
        printf("2. Replay sequence\n");
        printf("3. Analyze UART signal (GP4)\n");
        printf("4. Exit\n");
        printf("Choice: ");
        
        char choice = getchar();
        printf("\n");
        
        switch(choice) {
            case '1':
                if (read_stored_sequence()) {
                    printf("✓ Successfully loaded %d pulses\n", get_pulse_count());
                } else {
                    printf("✗ Failed to load complete sequence\n");
                }
                break;
                
            case '2':
                if (get_pulse_count() > 0) {
                    replay_sequence();
                } else {
                    printf("No sequence loaded! Read sequence first.\n");
                }
                break;
            case '3': {
                printf("Analyzing UART signal on GP4...\n");
                printf("Ensure test device is connected and transmitting\n");
                uart_result_t result = analyze_uart_signal(1000);
                display_uart_results(&result);
                break;
            }
            case '4':{
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