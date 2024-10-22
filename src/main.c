#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

#define PWM_PIN 2     // GP2 for PWM
#define DIR_PIN1 0    // GP0 for direction
#define DIR_PIN2 1    // GP1 for direction
#define PULSE_PIN 16  // GP16 for pulse generation
#define BUTTON_A 20   // GP20 for Button A (PWM change & Pulse ON)
#define BUTTON_B 21   // GP21 for Button B (PWM change & Pulse OFF)

#define PULSE_WIDTH 2000  // Pulse width in milliseconds (2 seconds)

volatile bool button_a_pressed = false;
volatile bool button_b_pressed = false;

void button_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_A) {
        button_a_pressed = true;
    } else if (gpio == BUTTON_B) {
        button_b_pressed = true;
    }
}

void setup_pwm(uint gpio, float freq, float duty_cycle) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    float clock_freq = 125000000.0f;
    uint32_t divider = clock_freq / (freq * 65536);
    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_gpio_level(gpio, (uint16_t)(duty_cycle * 65535));
    pwm_set_enabled(slice_num, true);
}

void generate_pulse(uint pin, uint32_t width_ms) {
    printf("Pulse HIGH for %u ms\n", width_ms);
    gpio_put(pin, 1);
    sleep_ms(width_ms);
    printf("Pulse LOW\n");
    gpio_put(pin, 0);
}

int main() {
    stdio_init_all();
    
    // Initialize motor control pins
    gpio_init(DIR_PIN1);
    gpio_init(DIR_PIN2);
    gpio_set_dir(DIR_PIN1, GPIO_OUT);
    gpio_set_dir(DIR_PIN2, GPIO_OUT);
    
    // Initialize pulse pin
    gpio_init(PULSE_PIN);
    gpio_set_dir(PULSE_PIN, GPIO_OUT);
    
    // Set up button interrupts
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_RISE, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_RISE, true, &button_callback);
    sleep_ms(2000); // Wait for initialization

    // Initial PWM setup
    setup_pwm(PWM_PIN, 100.0f, 0.5f); // Initial state: 100Hz, 50% duty cycle
    
    printf("System Ready\n");
    printf("Button A: Change PWM to 100Hz, 25%% & Generate %ums pulse\n", PULSE_WIDTH);
    printf("Button B: Change PWM to 200Hz, 50%% & Turn Pulse OFF\n");
    
    while (1) {
        // Forward direction
        gpio_put(DIR_PIN1, 1);
        gpio_put(DIR_PIN2, 0);
        
        if (button_a_pressed) {
            setup_pwm(PWM_PIN, 100.0f, 0.25f); // 100Hz, 25% duty cycle
            printf("PWM set to 100Hz, 25%% duty cycle\n");
            generate_pulse(PULSE_PIN, PULSE_WIDTH);  // Generate a 2-second pulse
            button_a_pressed = false;
        }
        
        if (button_b_pressed) {
            setup_pwm(PWM_PIN, 200.0f, 0.5f); // 200Hz, 50% duty cycle
            printf("PWM set to 200Hz, 50%% duty cycle\n");
            gpio_put(PULSE_PIN, 0);  // Ensure pulse is OFF
            button_b_pressed = false;
        }
        
        sleep_ms(100);  // Small delay for debouncing
    }
    
    return 0;
}