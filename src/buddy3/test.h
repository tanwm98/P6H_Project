// // signal_generator.h
// #ifndef TEST_H
// #define TEST_H

// #include <stdio.h>
// #include <stdbool.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "hardware/gpio.h"
// #include "hardware/timer.h"
// #include "hardware/uart.h"
// #include "hardware/irq.h"
// #include "pico/sync.h"

// // Pin definitions
// #define OUTPUT_PIN 3        // GP3 for pulse output
// #define UART_MONITOR_PIN 4  // GP4 for UART

// // I2C pins (using I2C1)
// #define I2C_SDA_PIN 6      // GPIO 6 (Pin 9)
// #define I2C_SCL_PIN 7      // GPIO 7 (Pin 10)

// // SPI pins (using SPI1)
// #define SPI_SCK_PIN 10     // GPIO 10 (Pin 14)
// #define SPI_MOSI_PIN 11    // GPIO 11 (Pin 15)
// #define SPI_MISO_PIN 12    // GPIO 12 (Pin 16)
// #define SPI_CS_PIN 13      // GPIO 13 (Pin 17)

// // Configuration constants
// #define MAX_EDGES 256
// #define MIN_EDGES_FOR_DETECTION 10
// #define TIMING_ERROR_THRESHOLD 50  // microseconds
// #define MAX_TIMING_ERROR_PCT 10.0f // 10% error margin

// // Protocol types
// typedef enum {
//     PROTOCOL_UNKNOWN = 0,
//     PROTOCOL_UART,
//     PROTOCOL_I2C,
//     PROTOCOL_SPI
// } protocol_type_t;

// // Common timing structure
// typedef struct {
//     uint32_t frequency;     // Hz for I2C/SPI, baud for UART
//     float error_margin;     // Percentage
//     bool is_valid;
// } timing_info_t;

// // Protocol-specific structures
// typedef struct {
//     uint32_t baud_rate;
//     uint8_t data_bits;
//     bool parity_enabled;
//     bool parity_even;
//     uint8_t stop_bits;
// } uart_config_t;

// typedef struct {
//     bool address_detected;
//     uint8_t address;
//     bool is_10bit;
// } i2c_config_t;

// typedef struct {
//     uint8_t mode;          // 0-3
//     bool cs_active_low;
//     bool msb_first;
// } spi_config_t;

// // Unified result structure
// typedef struct {
//     protocol_type_t type;
//     timing_info_t timing;
//     bool signal_detected;
//     union {
//         uart_config_t uart;
//         i2c_config_t i2c;
//         spi_config_t spi;
//     } config;
//     char debug_info[256];
// } protocol_result_t;

// // Status codes
// typedef enum {
//     SG_SUCCESS = 0,
//     SG_ERROR_INVALID_DATA,
//     SG_ERROR_TIMING,
//     SG_ERROR_NO_SIGNAL,
//     SG_ERROR_PROTOCOL_UNKNOWN
// } signal_status_t;

// // Pulse sequence structure (maintaining compatibility)
// typedef struct {
//     uint32_t timestamp;
//     bool level;
//     bool verified;
// } pulse_data_t;

// typedef struct {
//     pulse_data_t pulses[10];
//     uint8_t count;
//     bool sequence_verified;
// } sequence_data_t;

// // Function declarations
// // Initialization and cleanup
// void signal_generator_init(void);
// void protocol_detector_init(void);
// void cleanup_detection(void);

// // Core functionality
// signal_status_t reproduce_sequence(const sequence_data_t* sequence);
// protocol_result_t detect_protocol(uint32_t timeout_ms);
// void abort_detection(void);

// // Display and debug
// void display_protocol_results(const protocol_result_t* result);
// const char* get_status_string(signal_status_t status);
// void print_debug_info(const protocol_result_t* result);

// #endif // SIGNAL_GENERATOR_H