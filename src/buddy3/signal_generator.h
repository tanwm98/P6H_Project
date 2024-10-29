#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "hardware/irq.h"


// Pin definitions, change accoridngly
#define OUTPUT_PIN 3        // GP3 for pulse output
#define UART_MONITOR_PIN 4

// for final project deliverable
// #define MIN_SAMPLES_FOR_VALID_DETECTION 20
// #define BAUD_CALCULATION_SAMPLES 25
// #define MAX_FRAME_ERRORS 2

typedef struct {
    uint32_t baud_rate;
    uint32_t bit_time;
    float error_margin;
    bool is_valid;
    uint32_t sample_count;
    uint32_t frame_errors;
    uint32_t parity_errors;
    uint8_t data_bits;
    bool parity_enabled;
    bool parity_even;
    uint8_t stop_bits;
    uint32_t total_frames;
} uart_result_t;

// Frame analysis structure
typedef struct {
    uint32_t baud_rate;
    uint32_t bit_time;        // Measured bit time in microseconds
    uint32_t sample_count;    // Number of edges detected
    uint32_t frame_errors;    // Number of frame errors detected
    uint32_t parity_errors;   // Number of parity errors detected
    float error_margin;       // Error margin percentage
    bool is_valid;           // Overall validity of detection
    bool parity_enabled;     // Whether parity was detected
    uint8_t data_bits;       // Detected number of data bits
    uint8_t stop_bits;       // Detected number of stop bits
} uart_frame_t;

// Common baud rates for validation (based off serial monitor)
static const uint32_t STANDARD_BAUDS[] = {
    300, 1200, 2400, 4800, 9600, 19200, 
    38400, 57600, 74880, 115200
};

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

// Function declarations
void signal_generator_init(void);

// Pulse generation functions
signal_status_t reproduce_sequence(const sequence_data_t* sequence);
bool verify_sequence_timing(const sequence_data_t* sequence);
void abort_sequence(void);

// Updated function declarations
uart_result_t analyze_uart_signal(uint32_t timeout_ms);
bool verify_uart_frame(uart_frame_t* frame, uart_result_t* settings);
void analyze_frame_timing(uart_frame_t* frame, uart_result_t* settings);
void display_uart_results(const uart_result_t* result);

// Debug and status functions
const char* get_status_string(signal_status_t status);
void print_sequence_debug(const sequence_data_t* sequence);
void print_uart_debug(const uart_result_t* result);

#endif // SIGNAL_GENERATOR_H
