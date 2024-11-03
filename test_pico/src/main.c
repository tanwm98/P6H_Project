#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define UART_TX_PIN 0
#define PATTERN_REPEAT 50  // Number of times to repeat each pattern
#define VERIFY_PIN 3  // Pin to verify pulses

// Test patterns with guaranteed transitions
const uint8_t test_patterns[] = {
    0x55,  // Alternating 1/0 (good for baud rate detection)
    0xAA,  // Opposite alternating pattern
    0xF0,  // Slower transition pattern
    0x00,  // All zeros (tests start/stop bits)
    0xFF   // All ones (tests start/stop bits)
};

static const int BAUD_RATE = 115200;

void run_uart_tests(void) {
    printf("\nStarting UART Signal Generator\n");
    printf("Connect UART_TX (GP0) to analyzer input\n");
    
    // Initialize UART
    uart_init(uart0, BAUD_RATE);  // Start with 300 baud
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    
    printf("Sending test patterns at %d baud\n", BAUD_RATE);
    
    while(1) {
        for (int i = 0; i < sizeof(test_patterns); i++) {
            printf("Sending pattern 0x%02X (%d times)\n", 
                   test_patterns[i], PATTERN_REPEAT);
            
            // Send pattern multiple times with delay
            for (int j = 0; j < PATTERN_REPEAT; j++) {
                uart_putc(uart0, test_patterns[i]);
                sleep_ms(50);  // Add delay between characters
            }
            sleep_ms(1000);  // Pause between patterns
        }
        printf("\nPattern sequence complete. Repeating...\n");
        sleep_ms(2000);
    }
}

// Function to verify pulses on GP3
void verify_pulses(void) {
    printf("\n=== Pulse Verification on GP3 ===\n");
    
    gpio_init(VERIFY_PIN);
    gpio_set_dir(VERIFY_PIN, GPIO_IN);
    gpio_pull_down(VERIFY_PIN);  // Add pull-down resistor
    
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t last_change = time_us_32();
    bool last_state = gpio_get(VERIFY_PIN);
    uint transition_count = 0;
    bool timeout = false;
    
    printf("Starting verification...\n");    
    while (1) {  // Wait for exactly 20 transitions
        bool current_state = gpio_get(VERIFY_PIN);
        
        if (current_state != last_state) {
            uint32_t now = time_us_32();
            uint32_t interval = now - last_change;
            printf("Transition %2d: %4s at %lu us (interval: %lu us)\n",
                   ++transition_count,
                   current_state ? "HIGH" : "LOW",
                   now,
                   interval);
            last_change = now;
            last_state = current_state;
        }
        tight_loop_contents();
    }
    
    printf("\nVerification complete: %s\n", 
           timeout ? "TIMEOUT" : "COMPLETED");
    printf("Detected %d transitions\n", transition_count);
    if (transition_count != 20) {
        printf("ERROR: Expected 20 transitions (10 high + 10 low) but saw %d\n", 
               transition_count);
    } else {
        printf("SUCCESS: Detected all 20 transitions correctly\n");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for serial connection
    
    printf("\nTest Pico W - UART Tester\n");

    while(1) {
        printf("\nTest Menu:\n");
        printf("1. Run UART tests\n");
        printf("2. Verify pulses on GP3\n");
        printf("3. Exit\n");
        printf("Choice: ");
        
        char choice = getchar();
        printf("%c\n", choice);
        
        switch(choice) {
            case '1':
                run_uart_tests();
                break;
                
            case '2':
                verify_pulses();
                break;
                
            case '3':
                printf("Exiting...\n");
                return 0;
                
            default:
                printf("Invalid choice\n");
        }
        
        sleep_ms(1000);
    }
    
    return 0;
}