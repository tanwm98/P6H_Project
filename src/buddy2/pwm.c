#include "pwm.h"

static volatile PWMMetrics pwm_metrics = {0};

static void gpio_callback(uint gpio, uint32_t events) {
    static uint32_t last_button_time = 0;
    uint32_t now = time_us_32();

    // Handle button press
    if (gpio == PWM_BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        // Debounce - ignore events too close together
        if (now - last_button_time < 200000) { // 200ms debounce
            return;
        }
        last_button_time = now;

        if (!pwm_metrics.is_capturing) {
            start_capture();
        } else {
            stop_capture();
        }
        return;
    }

    // Handle PWM signal
    if (gpio == PWM_PIN && pwm_metrics.is_capturing) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            if (pwm_metrics.last_rise != 0) {
                pwm_metrics.period = now - pwm_metrics.last_rise;
                pwm_metrics.frequency = 1000000.0f / pwm_metrics.period;
            }
            pwm_metrics.last_rise = now;
        } 
        else if (events & GPIO_IRQ_EDGE_FALL) {
            if (pwm_metrics.last_rise != 0) {
                uint32_t high_time = now - pwm_metrics.last_rise;
                if (pwm_metrics.period > 0) {
                    pwm_metrics.duty_cycle = (float)high_time * 100.0f / pwm_metrics.period;
                }
            }
            pwm_metrics.last_fall = now;
            printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
                   pwm_metrics.frequency, pwm_metrics.duty_cycle);
        }
    }
}

void handle_pwm_edge(uint gpio, uint32_t events, uint32_t now) {
    if (events & GPIO_IRQ_EDGE_RISE) {
        if (pwm_metrics.last_rise != 0) {
            pwm_metrics.period = now - pwm_metrics.last_rise;
            pwm_metrics.frequency = 1000000.0f / pwm_metrics.period;
        }
        pwm_metrics.last_rise = now;
    } 
    else if (events & GPIO_IRQ_EDGE_FALL) {
        if (pwm_metrics.last_rise != 0) {
            uint32_t high_time = now - pwm_metrics.last_rise;
            if (pwm_metrics.period > 0) {
                pwm_metrics.duty_cycle = (float)high_time * 100.0f / pwm_metrics.period;
            }
        }
        pwm_metrics.last_fall = now;
        printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
               pwm_metrics.frequency, pwm_metrics.duty_cycle);
    }
}

// Modify pwm_analyzer_init() to remove the GPIO interrupt setup since it's now handled in main:
void pwm_analyzer_init(void) {
    // Initialize PWM input pin
    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN, GPIO_IN);
    
    // Initialize button with pull-up
    gpio_init(PWM_BUTTON_PIN);
    gpio_set_dir(PWM_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(PWM_BUTTON_PIN);
    
    // Remove the GPIO interrupt setup as it's now handled in main
    
    // Initialize metrics
    pwm_metrics.is_capturing = false;
    pwm_metrics.frequency = 0.0f;
    pwm_metrics.duty_cycle = 0.0f;
    pwm_metrics.last_rise = 0;
    pwm_metrics.last_fall = 0;
    pwm_metrics.period = 0;
}


PWMMetrics get_pwm_metrics(void) {
    PWMMetrics metrics = pwm_metrics;
    return metrics;
}

bool is_capturing(void) {
    return pwm_metrics.is_capturing;
}

void start_capture(void) {
    pwm_metrics.is_capturing = true;
    pwm_metrics.last_rise = 0;
    pwm_metrics.last_fall = 0;
    printf("Starting PWM capture...\n");
}

void stop_capture(void) {
    pwm_metrics.is_capturing = false;
    printf("Stopping PWM capture...\n");
}
