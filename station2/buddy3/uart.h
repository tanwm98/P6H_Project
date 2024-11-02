#ifndef UART_H
#define UART_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// UART-specific definitions
#define MIN_UART_EDGES 20
#define START_BIT_THRESHOLD 5    // Minimum number of start bits to detect UART
#define MAX_UART_ERROR 15.0f     // Maximum acceptable UART timing error %

// UART edge timing structure
typedef struct {
    uint32_t timestamp;
    bool level;
} UARTEdgeTiming;

// UART metrics structure
typedef struct {
    uint32_t baud_rate;
    float error_margin;
    uint32_t sample_count;
    bool is_valid;
    uint32_t parity_errors;
    uint32_t frame_errors;
    uint8_t data_bits;     // Usually 8
    uint8_t stop_bits;     // Usually 1
    bool parity_enabled;   // Whether parity checking is enabled
    char parity_type;      // 'N' (none), 'E' (even), 'O' (odd)
} UARTMetrics;

// Standard baud rates array declaration
extern const uint32_t STANDARD_BAUDS[];

// Function prototypes
void uart_analyzer_init(void);
bool uart_analyze_timing(UARTEdgeTiming* edge_buffer, uint32_t edge_count, UARTMetrics* metrics);
const char* uart_get_validity_string(bool is_valid);

// New UART analysis functions
bool uart_check_parity(uint8_t data, bool parity_bit, char parity_type);
bool uart_detect_frame_error(UARTEdgeTiming* edges, uint32_t start_index);
float uart_calculate_timing_error(uint32_t measured_interval, uint32_t expected_interval);

// Helper function to convert baud rate to bit period in microseconds
static inline uint32_t uart_get_bit_period(uint32_t baud_rate) {
    return 1000000 / baud_rate;
}

void uart_debug_edge(uint gpio, uint32_t events, uint32_t now);
void uart_debug_reset(void);
void uart_test_pin(void);
#endif