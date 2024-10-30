#ifndef PROTOCOL_ANALYZER_H
#define PROTOCOL_ANALYZER_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <math.h>

// Protocol types
typedef enum {
    PROTOCOL_UNKNOWN = 0,
    PROTOCOL_UART,
    PROTOCOL_I2C,
    PROTOCOL_SPI
} protocol_type_t;

// Generic protocol parameters
typedef struct {
    protocol_type_t type;
    bool is_valid;
    float error_margin;
    uint32_t sample_count;
} protocol_base_t;

// UART specific parameters
typedef struct {
    protocol_base_t base;
    uint32_t baud_rate;
    uint32_t bit_time_us;
    bool inverted;
} uart_params_t;

// I2C specific parameters (for future implementation)
typedef struct {
    protocol_base_t base;
    uint32_t clock_freq;
    uint8_t address;
    bool clock_stretch;
} i2c_params_t;

// SPI specific parameters (for future implementation)
typedef struct {
    protocol_base_t base;
    uint32_t clock_freq;
    uint8_t mode;        // SPI mode 0-3
    bool msb_first;
} spi_params_t;

// Union of all protocol parameters
typedef union {
    protocol_base_t base;
    uart_params_t uart;
    i2c_params_t i2c;
    spi_params_t spi;
} protocol_params_t;

// Edge timing structure
typedef struct {
    uint32_t timestamp;
    bool level;
} edge_timing_t;

// Analysis result structure
typedef struct {
    protocol_type_t detected_protocol;
    protocol_params_t params;
    bool is_valid;
    char error_message[64];
} protocol_result_t;

// Function declarations
void protocol_analyzer_init(uint uart_pin, uint i2c_scl, uint i2c_sda, uint spi_sck, uint spi_mosi, uint spi_miso);
protocol_result_t analyze_protocol(uint32_t timeout_ms);
void display_protocol_results(const protocol_result_t* result);
const char* get_protocol_name(protocol_type_t type);

#endif