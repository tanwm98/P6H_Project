#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <stdio.h>

#define BUTTON_PIN 21
#define PWM_INPUT_PIN 7

// Global variables for PWM metrics
volatile bool capturing = false;
volatile uint32_t last_rise_time = 0;
volatile uint32_t period = 0;
volatile uint32_t high_time = 0;
volatile float frequency = 0.0f;
volatile float duty_cycle = 0.0f;

// Button callback function to toggle capturing
void button_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        capturing = !capturing;
        if (capturing) {
            printf("PWM capture started on GP%d\n", PWM_INPUT_PIN);
            period = 0;
            high_time = 0;
        } else {
            printf("PWM capture stopped on GP%d\n", PWM_INPUT_PIN);
        }
    }
}

// PWM input callback to calculate frequency and duty cycle
void pwm_callback(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();

    if (gpio == PWM_INPUT_PIN && capturing) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            // Calculate period if it's not the first rising edge
            if (last_rise_time > 0) {
                period = now - last_rise_time;
                frequency = 1000000.0f / period;  // Frequency in Hz
                printf("Rising edge detected, period: %u us\n", period);
            }
            last_rise_time = now;  // Update last rise time
        }
        else if (events & GPIO_IRQ_EDGE_FALL) {
            // Calculate high time
            high_time = now - last_rise_time;
            if (period > 0) {
                duty_cycle = (high_time * 100.0f) / period;
                printf("Falling edge detected, high time: %u us\n", high_time);
            }
        }
    }
}

int main() {
    // Initialize stdio
    stdio_init_all();

    // Initialize button GPIO with an interrupt
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, button_callback);

    // Initialize PWM input GPIO with interrupts for rising and falling edges
    gpio_init(PWM_INPUT_PIN);
    gpio_set_dir(PWM_INPUT_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PWM_INPUT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, pwm_callback);

    while (true) {
        if (capturing && period > 0) {
            // Display frequency and duty cycle
            printf("Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", frequency, duty_cycle);

            // Reset values for the next capture cycle
            period = 0;
            high_time = 0;

            sleep_ms(500);  // Adjust capture interval as needed
        } else if (!capturing) {
            printf("Capture is inactive. Press GP21 to start.\n");
            sleep_ms(1000);  // Delay when not capturing
        }
    }

    return 0;
}
