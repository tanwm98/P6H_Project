#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define OUTPUT_PIN 3        // GP3 for pulse output
#define MAX_PULSES 10

typedef struct {
    uint32_t timestamp_ms;
    bool level;
} pulse_data_t;

// Function declarations
void signal_generator_init(void);
bool parse_pulse_line(const char* line, pulse_data_t* pulse);
bool read_stored_sequence();
void replay_sequence();
int get_pulse_count(void);

#endif