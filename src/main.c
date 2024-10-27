#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "buddy3/signal_generator.h"


int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\nSignal Generator & UART Analyzer - Week 10 Test\n");
    signal_generator_init();
    
    // Test sequence
    sequence_data_t test_sequence = {
        .pulses = {
            {0,      true},
            {1000,   false},
            {2000,   true},
            {3000,   false},
            {4000,   true},
            {5000,   false},
            {6000,   true},
            {7000,   false},
            {8000,   true},
            {9000,   false}
        },
        .count = 10,
        .sequence_verified = false
    };
    
    // Main test loop
    while (1) {
        printf("\nTest Menu:\n");
        printf("1. Generate pulse sequence\n");
        printf("2. Analyze UART signal\n");
        printf("3. Exit\n");
        printf("Choice: ");
        
        char choice = getchar();
        printf("\n");
        
        switch (choice) {
            case '1':
                printf("Running pulse sequence test...\n");
                signal_status_t status = reproduce_sequence(&test_sequence);
                
                // Direct status handling
                switch(status) {
                    case SG_SUCCESS:
                        printf("Sequence completed successfully\n");
                        break;
                    case SG_ERROR_INVALID_DATA:
                        printf("Error: Invalid sequence data\n");
                        break;
                    case SG_ERROR_TIMING:
                        printf("Error: Timing requirements not met\n");
                        break;
                    case SG_ERROR_SEQUENCE_FAIL:
                        printf("Error: Sequence generation failed\n");
                        break;
                    default:
                        printf("Error: Unknown status code\n");
                }
                break;
            case '2': {
                uart_result_t result = analyze_uart_signal(1000);
                if (result.is_valid) {
                    printf("UART Analysis: %lu baud (±%.1f%%)\n", 
                           result.baud_rate, result.error_margin);
                } else {
                    printf("UART Analysis failed\n");
                }
                break;
            }
            case 'q':
                return 0;
            default:
                printf("Invalid choice\n");
        }
        
        sleep_ms(1000);
    }
    
    return 0;
}


// Version 2: Integration-Ready Main (Theoretical Overview)
/*
// External component interfaces (provided by other buddies)
extern void dashboard_update_uart(uart_result_t result);  // Buddy 5
extern sequence_data_t* storage_get_sequence(void);       // Buddy 1
extern void signal_task(void);                           // Task scheduler

// Main system tasks
void uart_analysis_task(void) {
    uart_result_t result = analyze_uart_signal(1000);
    if (result.is_valid) {
        dashboard_update_uart(result);  // Send to dashboard
    }
}

void pulse_generation_task(void) {
    sequence_data_t* sequence = storage_get_sequence();  // Get from storage
    if (sequence != NULL) {
        reproduce_sequence(sequence);
    }
}

// Main function for integrated system
int main() {
    // System initialization
    stdio_init_all();
    signal_generator_init();
    
    // Register tasks with scheduler
    register_task(uart_analysis_task, 100);    // Run every 100ms
    register_task(pulse_generation_task, 500); // Run every 500ms
    
    // Start scheduler
    start_scheduler();
    
    while (1) {
        __wfi();  // Wait for interrupt
    }
}
*/