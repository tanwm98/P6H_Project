#include "signal_generator.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define SIG_GEN_PIN 16

void signal_generator_init() {
    gpio_init(SIG_GEN_PIN);
    gpio_set_dir(SIG_GEN_PIN, GPIO_OUT);
}

void generate_signal(uint32_t frequency, uint32_t duration_ms) {
    // i'll thikn of what to do here later
