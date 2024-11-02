#ifndef PWM_H
#define PWM_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <stdbool.h>

#define PWM_PIN 7
#define BUTTON_PIN 20

typedef struct {
    float frequency;
    float duty_cycle;
    uint32_t last_rise;
    uint32_t last_fall;
    uint32_t period;
    bool is_capturing;
} PWMMetrics;

void pwm_analyzer_init(void);
PWMMetrics get_pwm_metrics(void);
bool is_capturing(void);
void start_capture(void);
void stop_capture(void);
void handle_pwm_edge(uint gpio, uint32_t events, uint32_t now);

#endif // PWM_ANALYZER_H
