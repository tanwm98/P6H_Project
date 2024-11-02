// protocol_analyzer.h
#ifndef PROTOCOL_ANALYZER_H
#define PROTOCOL_ANALYZER_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Pin definitions
#define UART_RX_PIN      4
#define I2C_SCL_PIN      2
#define I2C_SDA_PIN      3
#define SPI_SCK_PIN      10
#define SPI_MOSI_PIN     11
#define SPI_MISO_PIN     12
#define PROTOCOL_BUTTON_PIN 22

// Buffer and timing constraints
#define MAX_EDGES 1000
#define MIN_EDGES_FOR_VALID 20
#define MAX_ERROR_MARGIN 10.0f  // Maximum acceptable error percentage

// Protocol types
typedef enum {
    PROTOCOL_UNKNOWN = 0,
    PROTOCOL_UART,
    PROTOCOL_I2C,
    PROTOCOL_SPI
} protocol_type_t;

// Edge timing structure
typedef struct {
    uint32_t timestamp;  // Microseconds
    bool level;          // Signal level
} edge_timing_t;

// Protocol metrics structure
typedef struct {
    volatile bool is_capturing;
    volatile uint32_t edge_count;
    edge_timing_t edge_buffer[MAX_EDGES];
    protocol_type_t detected_protocol;
    bool is_valid;
    uint32_t baud_rate;        // For UART
    float error_margin;        // Error percentage
    uint32_t sample_count;     // Number of valid samples
    uint32_t parity_errors;    // UART parity errors
    uint32_t frame_errors;     // UART frame errors
} ProtocolMetrics;

// Add protocol-specific pattern detection
typedef struct {
    uint32_t uart_start_bits;
    uint32_t i2c_start_conditions;
    uint32_t spi_clock_edges;
    uint32_t parity_errors;
    uint32_t frame_errors;
} protocol_patterns_t;

// Function prototypes
void protocol_analyzer_init(void);
void handle_protocol_edge(uint gpio, uint32_t events, uint32_t now);
void start_protocol_capture(void);
void stop_protocol_capture(void);
bool is_protocol_capturing(void);
bool is_protocol_valid(void);
const char* get_protocol_name(protocol_type_t type);

// Getter functions
float get_error_margin(void);
uint32_t get_baud_rate(void);
protocol_type_t get_detected_protocol(void);
uint32_t get_sample_count(void);
uint32_t get_edge_count(void);
void set_edge_count(uint32_t count);

// Protocol-specific analysis functions
static void identify_protocol(void);
static void analyze_protocol_data(void);
static void analyze_uart_data(void);
// static void analyze_i2c_data(void);
// static void analyze_spi_data(void);



#define MIN_EDGE_TIME_US  100  // Minimum time between valid edges
#define MAX_EDGE_TIME_US  100000  // Maximum time for valid UART at 300 baud












#endif // PROTOCOL_ANALYZER_H