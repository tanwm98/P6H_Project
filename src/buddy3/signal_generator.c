#include "signal_generator.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define SIG_GEN_PIN 16

void signal_generator_init() {
    gpio_init(SIG_GEN_PIN);
    gpio_set_dir(SIG_GEN_PIN, GPIO_OUT);
}

void generate_signal(uint32_t frequency, uint32_t duration_ms) {
    uint32_t delay_us = 1000000 / (frequency * 2); // 50% duty cycle

    uint32_t cycles = (duration_ms * 1000) / (2 * delay_us);
    for(uint32_t i = 0; i < cycles; i ++)
    {
        gpio_put(SIG_GEN_PIN, 1);
        sleep_us(delay_us);
        gpio_put(SIG_GEN_PIN, 0);
        sleep_us(delay_us);
    }
}
