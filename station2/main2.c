#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy2/adc.h"
#include "buddy2/pwm.h"
#include "buddy3/protocol_analyzer.h"

static void display_menu(void) {
    printf("\nSystem Ready:\n");
    printf("- PWM Analysis (GP7) - Button GP20\n");
    printf("- ADC Analysis (GP26) - Button GP21\n");
    printf("- Protocol Analysis - Button GP22\n");
    printf("  * UART RX: GP4\n");
    // printf("  * I2C SCL: GP8, SDA: GP9\n");
    // printf("  * SPI SCK: GP10, MOSI: GP11, MISO: GP12\n");
    printf("Press respective buttons to start/stop capture\n\n");
}

// Unified GPIO callback
static void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();
    static uint32_t last_button_time = 0;
    
    // Button handling with debouncing
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Debounce all buttons - ignore events too close together
        if (now - last_button_time < 200000) { // 200ms debounce
            return;
        }
        last_button_time = now;

        // PWM Button (GP21)
        if (gpio == PWM_BUTTON_PIN) {
            if (!is_capturing()) {
                start_capture();
            } else {
                stop_capture();
                display_menu();
            }
        }
        // ADC Button (GP20)
        else if (gpio == ADC_BUTTON_PIN) {
            if (!is_adc_capturing()) {
                adc_start_capture();
            } else {
                adc_stop_capture();
                display_menu();
            }
        }
        // Protocol Button (GP22)
        else if (gpio == PROTOCOL_BUTTON_PIN) {
            if (!is_protocol_capturing()) {
                start_protocol_capture();
            } else {
                stop_protocol_capture();
                display_menu();
            }
        }
    }

    // PWM Signal (GP7)
    if (gpio == PWM_PIN && is_capturing()) {
        handle_pwm_edge(gpio, events, now);
    }
    
    // Protocol Analysis Signals
    if ((gpio == UART_RX_PIN || gpio == I2C_SCL_PIN || gpio == I2C_SDA_PIN ||
         gpio == SPI_SCK_PIN || gpio == SPI_MOSI_PIN || gpio == SPI_MISO_PIN) &&
        is_protocol_capturing()) {
        handle_protocol_edge(gpio, events, now);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\nIntegrated Signal Analyzer Program\n");
    printf("================================\n");
    
    // Initialize all GPIO interrupts with unified callback
    gpio_set_irq_enabled_with_callback(PWM_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Initialize all modules
    adc_analyzer_init();
    pwm_analyzer_init();
    protocol_analyzer_init();

    // Enable interrupts for other pins without callback
    gpio_set_irq_enabled(PWM_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);  // PWM button
    gpio_set_irq_enabled(ADC_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);  // ADC button
    
    // Enable protocol analysis pin interrupts
    gpio_set_irq_enabled(UART_RX_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(I2C_SCL_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(I2C_SDA_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SPI_SCK_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SPI_MOSI_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SPI_MISO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PROTOCOL_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);

    display_menu();

    // Main loop
    while (1) {
        sleep_ms(1000);
        
        // Display PWM status
        if (is_capturing()) {
            PWMMetrics pwm = get_pwm_metrics();
            printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
                   pwm.frequency, pwm.duty_cycle);
        }
        
        // Display ADC status
        if (is_adc_capturing() && is_transfer_complete()) {
            clear_transfer_complete();
            float freq = analyze_current_capture();
            if (freq > 0) {
                printf("ADC - Frequency: %.1f Hz\n", freq);
            }
        }
    }
    return 0;
}