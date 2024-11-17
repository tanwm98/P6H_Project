#ifndef DIGITAL_H
#define DIGITAL_H

#include "pico/stdlib.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define DIGITAL_INPUT_PIN 2    // GP2 for capturing pulses
#define DIGITAL_OUTPUT_PIN 3   // GP3 for replaying pulses
#define MAX_TRANSITIONS 20     // 10 highs + 10 lows = 20 total transitions
#define PULSE_FILE "pulses.csv"

// Structure to store transition timing information
typedef struct {
    uint32_t time;      // Time of transition
    bool state;         // true = high, false = low
} Transition;

// Structure to manage pulse capture state
typedef struct {
    Transition transitions[MAX_TRANSITIONS];
    uint8_t transition_count;
    bool capturing;
    uint32_t start_time;
    bool expecting_high;  // Track which edge we're expecting next
} PulseCapture;

// Function declarations
void digital_init(void);
void start_pulse_capture(void);
bool is_capture_complete(void);
const Transition* get_captured_transitions(uint8_t* count);
void replay_pulses(uint8_t num_times);
void save_pulses_to_file(const char* filename);
bool load_pulses_from_file(const char* filename);
void gpio_callback_digital(uint gpio, uint32_t events); 
void handle_capture_completion(void);
bool should_stop_capture(void);
bool is_capture_complete(void);
#endif // DIGITAL_H