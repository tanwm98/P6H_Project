// protocol_analyzer.h
#ifndef PROTOCOL_ANALYZER_H
#define PROTOCOL_ANALYZER_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <math.h>

#define UART_RX_PIN 4 
#define I2C_SCL_PIN 8     
#define I2C_SDA_PIN 9 
#define SPI_SCK_PIN 10  
#define SPI_MOSI_PIN 11  
#define SPI_MISO_PIN 12   
#define PROTOCOL_BUTTON_PIN 22

#define MAX_EDGES 256
#define MIN_EDGES_FOR_VALID 20

// Protocol types (kept from original)
typedef enum {
    PROTOCOL_UNKNOWN = 0,
    PROTOCOL_UART,
    PROTOCOL_I2C,
    PROTOCOL_SPI
} protocol_type_t;

// Edge timing structure
typedef struct {
    uint32_t timestamp;
    bool level;
} edge_timing_t;

// Protocol Metrics structure (similar to PWMMetrics)
typedef struct {
    protocol_type_t detected_protocol;
    uint32_t baud_rate;        // For UART
    uint32_t clock_freq;       // For I2C/SPI
    float error_margin;
    uint32_t sample_count;
    bool is_capturing;
    bool is_valid;
    char error_message[64];
    edge_timing_t edge_buffer[MAX_EDGES];
    uint32_t edge_count;
} ProtocolMetrics;

// Function declarations (simplified like PWM analyzer)
void protocol_analyzer_init(void);
ProtocolMetrics get_protocol_metrics(void);
bool is_protocol_capturing(void);
void start_protocol_capture(void);
void stop_protocol_capture(void);
void handle_protocol_edge(uint gpio, uint32_t events, uint32_t now);
const char* get_protocol_name(protocol_type_t type);
static bool analyze_uart_timing(void);
static void analyze_captured_data(void);

#endif

// protocol_analyzer.c