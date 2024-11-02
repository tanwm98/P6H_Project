// main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy2/adc.h"
#include "buddy2/pwm.h"
#include "buddy3/uart.h"
#include "buddy3/spi.h"
#include "buddy3/i2c.h"
#include "buddy3/protocol_analyzer.h"


#define DEBOUNCE_US 200000  // 200ms debounce time
extern void display_menu(void);  // Make display_menu accessible to protocol analyzer

// Enum to track capture types
typedef enum {
    CAPTURE_NONE = 0,
    CAPTURE_PWM,
    CAPTURE_ADC,
    CAPTURE_PROTOCOL
} capture_type_t;

// Global variable to track active capture
static volatile capture_type_t active_capture = CAPTURE_NONE;

// Struct for button debouncing
typedef struct {
    uint gpio;
    uint32_t last_press_time;
} button_state_t;

// Array of button states for debouncing
static button_state_t buttons[] = {
    {PWM_BUTTON_PIN, 0},        // PWM button
    {ADC_BUTTON_PIN, 0},// ADC button
    {PROTOCOL_BUTTON_PIN, 0}// Protocol button
};

// Helper function to find button index
static int find_button_index(uint gpio) {
    for (int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        if (buttons[i].gpio == gpio) return i;
    }
    return -1;
}

// Helper function to check if button press is valid (not bouncing)
static bool is_valid_button_press(int button_index, uint32_t now) {
    if (button_index < 0) return false;
    
    if (now - buttons[button_index].last_press_time < DEBOUNCE_US) {
        return false;
    }
    
    buttons[button_index].last_press_time = now;
    return true;
}

// Helper function to stop current capture
static void stop_current_capture(void) {
    switch (active_capture) {
        case CAPTURE_PWM:
            stop_capture();
            break;
        case CAPTURE_ADC:
            adc_stop_capture();
            break;
        case CAPTURE_PROTOCOL:
            stop_protocol_capture();
            break;
        default:
            break;
    }
    // IMPORTANT: Always reset active_capture state
    active_capture = CAPTURE_NONE;
}

static bool start_new_capture(capture_type_t type) {
    // First check if we need to stop any existing capture
    if (active_capture != CAPTURE_NONE) {
        stop_current_capture();
    }
    
    // Now we can safely start the new capture
    active_capture = type;
    return true;
}


extern void display_menu(void) {
    printf("\nSystem Ready:\n");
    printf("- PWM Analysis (GP7) - Button GP20\n");
    printf("- ADC Analysis (GP26) - Button GP21\n");
    printf("- Protocol Analysis - Button GP22\n");
    printf("  * UART RX: GP4 (Auto Baud Detection)\n");
    printf("Press respective buttons to start/stop capture\n");
}

static void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();
    int button_index;
    
    if (events & GPIO_IRQ_EDGE_FALL) {
        button_index = find_button_index(gpio);
        if (!is_valid_button_press(button_index, now)) {
            return;
        }

        if (gpio == PWM_BUTTON_PIN) {
            if (!is_capturing()) {
                if (start_new_capture(CAPTURE_PWM)) {
                    start_capture();
                }
            } else {
                stop_current_capture();
                display_menu();
            }
        }
        else if (gpio == ADC_BUTTON_PIN) {
            if (!is_adc_capturing()) {
                if (start_new_capture(CAPTURE_ADC)) {
                    adc_start_capture();
                }
            } else {
                stop_current_capture();
                display_menu();
            }
        }
        else if (gpio == PROTOCOL_BUTTON_PIN) {
            if (!is_protocol_capturing() && active_capture == CAPTURE_NONE) {
                if (start_new_capture(CAPTURE_PROTOCOL)) {
                    start_protocol_capture();
                    printf("\nStarted Protocol analysis - Waiting for signal...\n");
                    printf("Press GP22 again to stop\n");
                }
            } else {
                stop_current_capture();
                printf("\nStopped Protocol analysis\n");
                display_menu();
            }
        }
    }
    // Only process edges if the correct capture is active
    if (gpio == PWM_PIN && active_capture == CAPTURE_PWM) {
        handle_pwm_edge(gpio, events, now);
    }
    
    if (active_capture == CAPTURE_PROTOCOL) {
        if (gpio == UART_RX_PIN && (events & (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL))) {
            uart_debug_edge(gpio, events, now);  // Add debug monitoring
            handle_protocol_edge(gpio, events, now);
        }
    }
}


int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\nIntegrated Signal Analyzer Program\n");
    printf("================================\n");
    
    gpio_set_irq_enabled_with_callback(PWM_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    adc_analyzer_init();
    pwm_analyzer_init();
    protocol_analyzer_init();
    gpio_set_irq_enabled(PWM_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ADC_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PROTOCOL_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);    
    gpio_set_irq_enabled(UART_RX_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);  // Add this line

    display_menu();

    while (1) {
        sleep_ms(1000);
        
        // Only display metrics for the active capture type
        switch (active_capture) {
            case CAPTURE_PWM:
                if (is_capturing()) {
                    PWMMetrics pwm = get_pwm_metrics();
                    printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
                           pwm.frequency, pwm.duty_cycle);
                }
                break;
                
            case CAPTURE_ADC:
                if (is_adc_capturing() && is_transfer_complete()) {
                    clear_transfer_complete();
                    float freq = analyze_current_capture();
                    if (freq > 0) {
                        printf("ADC - Frequency: %.1f Hz\n", freq);
                    }
                }
                break;
                
            case CAPTURE_PROTOCOL: {
                uint32_t now = to_ms_since_boot(get_absolute_time());
                static uint32_t last_print = 0;
                if (is_protocol_capturing()) {
                    if (is_protocol_valid()) {
                        // Only print updates periodically to avoid console spam
                        if (now - last_print >= 500) {
                            if (get_detected_protocol() == PROTOCOL_UART) {
                                printf("\rUART Analysis - Baud: %lu, Error: %.1f%%, Frames: %lu   ", 
                                    get_baud_rate(), 
                                    get_error_margin(),
                                    get_sample_count());
                                fflush(stdout);
                            }
                            last_print = now;
                        }
                    }
                }
                break;
            }
        }
    }
    return 0;
}