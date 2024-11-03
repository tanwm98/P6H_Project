#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy2/adc.h"
#include "buddy2/pwm.h"
#include "buddy3/protocol_analyzer.h"
#include "buddy4/swd.h"
#include "buddy5/wifi_dashboard.h"

static void display_menu(void);
static void gpio_callback(uint gpio, uint32_t events);
static void handle_dashboard_command(const char* cmd);
static DashboardData dashboard_data = {0};

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

static void handle_dashboard_command(const char* cmd) {
    if (strcmp(cmd, "halt") == 0) {
        printf("Received halt command\n");
        dashboard_data.device_halted = true;
        //TODO: Add your halt implementation here
    } 
    else if (strcmp(cmd, "resume") == 0) {
        printf("Received resume command\n");
        dashboard_data.device_halted = false;
        //TODO: Add your resume implementation here
    }
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

    swd_init();
    uint32_t idcode = read_idcode();
    printf("IDCODE: 0x%08X\n", idcode);
    dashboard_data.idcode = idcode;
    printf("Main: Dashboard IDCODE set to: 0x%08X\n", dashboard_data.idcode);


    if (!init_wifi_dashboard()) {
        printf("Failed to initialize WiFi dashboard\n");
        return -1;
    }
    register_dashboard_callback(handle_dashboard_command);
    
    // Initialize all GPIO interrupts with unified callback
    gpio_set_irq_enabled_with_callback(PWM_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Initialize all modules
    adc_analyzer_init();
    pwm_analyzer_init();
    protocol_analyzer_init();

    // Enable interrupts for other pins without callback
    gpio_set_irq_enabled(PWM_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ADC_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    
    // Enable protocol analysis pin interrupts
    gpio_set_irq_enabled(UART_RX_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(I2C_SCL_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(I2C_SDA_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(SPI_SCK_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(SPI_MOSI_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(SPI_MISO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PROTOCOL_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);

    display_menu();

    // Main loop
    while (1) {
        // Update dashboard data
        if (is_capturing()) {
            PWMMetrics pwm = get_pwm_metrics();
            dashboard_data.pwm_frequency = pwm.frequency;
            dashboard_data.pwm_duty_cycle = pwm.duty_cycle;
            
            printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
                   pwm.frequency, pwm.duty_cycle);
        }
        
        if (is_adc_capturing() && is_transfer_complete()) {
            clear_transfer_complete();
            float freq = analyze_current_capture();
            if (freq > 0) {
                dashboard_data.analog_frequency = freq;
                printf("ADC - Frequency: %.1f Hz\n", freq);
            }
        }
        
        // Update UART baud rate if protocol analyzer is running
        if (is_protocol_capturing()) {
            float baud_rate = get_uart_baud_rate();
            if (baud_rate > 0) {
                // Only update if we have a valid reading
                dashboard_data.uart_baud_rate = baud_rate;
                // Optional: Add hysteresis to prevent rapid updates
                static float last_reported_baud = 0;
                if (fabs(baud_rate - last_reported_baud) > 100) { // Threshold for reporting change
                    printf("UART Baud Rate: %.0f\n", baud_rate);
                    last_reported_baud = baud_rate;
                }
            }
        }
        
        // Update dashboard data and handle events
        update_dashboard_data(&dashboard_data);
        handle_dashboard_events();
        
        sleep_ms(1000); // Reduced sleep time for more responsive updates
    }
    return 0;
}