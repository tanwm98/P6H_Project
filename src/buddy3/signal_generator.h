// signal_generator.h
#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// Pin definitions as per requirements
#define OUTPUT_PIN 3        // GP3 for pulse output
#define UART_MONITOR_PIN 4  // GP4 for UART analysis

// Status codes for better error handling
typedef enum {
    SG_SUCCESS = 0,
    SG_ERROR_INVALID_DATA,
    SG_ERROR_TIMING,
    SG_ERROR_SEQUENCE_FAIL,
    SG_ERROR_UART_TIMEOUT,
    SG_ERROR_UART_INVALID
} signal_status_t;

// Pulse sequence structure
typedef struct {
    uint32_t timestamp;    // Microsecond timestamp
    bool level;           // Signal level
    bool verified;        // For verification
} pulse_data_t;

typedef struct {
    pulse_data_t pulses[10];
    uint8_t count;
    bool sequence_verified;
} sequence_data_t;

// UART analysis results
typedef struct {
    uint32_t baud_rate;
    uint32_t bit_time;     // Measured bit time
    float error_margin;    // Error margin percentage
    bool is_valid;
    uint32_t sample_count; // Number of samples used
} uart_result_t;

// Function declarations
void signal_generator_init(void);

// Pulse generation functions
signal_status_t reproduce_sequence(const sequence_data_t* sequence);
bool verify_sequence_timing(const sequence_data_t* sequence);
void abort_sequence(void);

// UART analysis functions
uart_result_t analyze_uart_signal(uint32_t timeout_ms);
bool is_valid_baud_rate(uint32_t baud_rate);

// Debug and status functions
const char* get_status_string(signal_status_t status);
void print_sequence_debug(const sequence_data_t* sequence);
void print_uart_debug(const uart_result_t* result);

#endif // SIGNAL_GENERATOR_H
