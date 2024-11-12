#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy1/sd_card.h"
#include "buddy2/digital.h"
#include "buddy2/adc.h"
#include "buddy2/pwm.h"
#include "buddy3/protocol_analyzer.h"
#include "buddy4/swd.h"
#include "buddy5/wifi.h"

// File name for storing pulse data
#define PULSE_FILE "pulses.csv"

// Global variables
static DashboardData dashboard_data = {0};
bool capture_handled = true;

// Function prototypes
static void handle_dashboard_command(const char* cmd);
static void gpio_callback(uint gpio, uint32_t events);
void process_command(char cmd);

// Dashboard command handler
static void handle_dashboard_command(const char* cmd) {
    if (strcmp(cmd, "capture_pulses") == 0) {
        start_pulse_capture();
        capture_handled = false;
        dashboard_data.capture_active = true;
    }
    else if (strcmp(cmd, "replay_pulses") == 0) {
        if (load_pulses_from_file(PULSE_FILE)) {
            replay_pulses(2);
            dashboard_data.replay_active = true;
        }
    }
    update_dashboard_data(&dashboard_data);
}

// Unified GPIO callback
static void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();
    static uint32_t last_button_time = 0;
    
    // Debounce logic
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (now - last_button_time < 200000) { // 200ms debounce
            return;
        }
        last_button_time = now;

        // PWM Button handler
        if (gpio == PWM_BUTTON_PIN) {
            if (!is_capturing()) {
                start_capture();
                dashboard_data.pwm_active = true;
            } else {
                stop_capture();
                dashboard_data.pwm_active = false;
            }
        }
        // ADC Button handler 
        else if (gpio == ADC_BUTTON_PIN) {
            if (!is_adc_capturing()) {
                adc_start_capture();
                dashboard_data.adc_active = true;
            } else {
                adc_stop_capture();
                dashboard_data.adc_active = false;
            }
        }
        // Protocol Button handler
        else if (gpio == PROTOCOL_BUTTON_PIN) {
            if (!is_protocol_capturing()) {
                start_protocol_capture();
                dashboard_data.protocol_active = true;
            } else {
                stop_protocol_capture();
                dashboard_data.protocol_active = false;
            }
        }
        update_dashboard_data(&dashboard_data);
    }

    // Signal handlers
    if (gpio == PWM_PIN && is_capturing()) {
        handle_pwm_edge(gpio, events, now);
    }
    
    if ((gpio == UART_RX_PIN || gpio == I2C_SCL_PIN || gpio == I2C_SDA_PIN ||
         gpio == SPI_SCK_PIN || gpio == SPI_MOSI_PIN || gpio == SPI_MISO_PIN) &&
        is_protocol_capturing()) {
        handle_protocol_edge(gpio, events, now);
    }
}

int main() {
    // Initialize stdio and USB
    stdio_init_all();
    sleep_ms(2000);

    // Initialize NTP time sync
    if (!wifi()) {
        dashboard_data.error_state = true;
        dashboard_data.error_message = "NTP sync failed";
        update_dashboard_data(&dashboard_data);
        while (true) {
            tight_loop_contents();
        }
    }

    register_dashboard_callback(handle_dashboard_command);

    // Initialize SD card
    if (FR_OK != initialiseSD()) {
        dashboard_data.error_state = true;
        dashboard_data.error_message = "SD card init failed";
        update_dashboard_data(&dashboard_data);
        while (true) {
            tight_loop_contents();
        }
    }

    // Initialize SWD and get IDCODE
    swd_init();
    dashboard_data.idcode = read_idcode();

    // Initialize all systems
    digital_init();
    //adc_analyzer_init();
    pwm_analyzer_init();
    protocol_analyzer_init();

    // Set up GPIO interrupts
    gpio_set_irq_enabled_with_callback(PWM_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    
    gpio_set_irq_enabled(PWM_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ADC_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(UART_RX_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PROTOCOL_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true);

    // Main loop
    while (true) {
        // Handle pulse capture completion
        if (!capture_handled && is_capture_complete()) {
            save_pulses_to_file(PULSE_FILE);
            dashboard_data.capture_active = false;
            capture_handled = true;
            update_dashboard_data(&dashboard_data);
        }

        // Update PWM metrics
        if (is_capturing()) {
            PWMMetrics pwm = get_pwm_metrics();
            dashboard_data.pwm_frequency = pwm.frequency;
            dashboard_data.pwm_duty_cycle = pwm.duty_cycle;
            update_dashboard_data(&dashboard_data);
        }
        
        // Update ADC analysis
        if (is_adc_capturing() && is_transfer_complete()) {
            clear_transfer_complete();
            float freq = analyze_current_capture();
            if (freq > 0) {
                dashboard_data.analog_frequency = freq;
                update_dashboard_data(&dashboard_data);
            }
        }
        
        // Update protocol analysis
        if (is_protocol_capturing()) {
            float baud_rate = get_uart_baud_rate();
            if (baud_rate > 0) {
                static float last_reported_baud = 0;
                if (fabs(baud_rate - last_reported_baud) > 100) {
                    dashboard_data.uart_baud_rate = baud_rate;
                    last_reported_baud = baud_rate;
                    update_dashboard_data(&dashboard_data);
                }
            }
        }

        // Handle dashboard events
        handle_wifi_events();
        
        sleep_ms(100); // Reduced sleep time for more responsive updates
    }

    return 0;
}