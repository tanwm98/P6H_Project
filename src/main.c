#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "buddy3/signal_generator.h"
#include "buddy3/protocol_analyzer.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\nPico Pirate - Week 10\n");
    
    // Initialize both modules
    signal_generator_init();
    protocol_analyzer_init(4,    // UART pin (GP4)
                         5,      // I2C SCL
                         6,      // I2C SDA
                         10,     // SPI SCK
                         11,     // SPI MOSI
                         12);    // SPI MISO
    
    while (1) {
        printf("\nTest Menu:\n");
        printf("1. Read stored sequence\n");
        printf("2. Replay sequence\n");
        printf("3. Analyze protocol signals\n");
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
                printf("Analyzing signals...\n");
                protocol_result_t result = analyze_protocol(1000);
                display_protocol_results(&result);
                break;
            }
            
            case '4': {
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
