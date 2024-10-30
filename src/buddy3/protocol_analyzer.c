#include "protocol_analyzer.h"

#define MAX_EDGES 256
#define MIN_EDGES_FOR_VALID 20
#define MIN_UART_EDGES 20
#define MIN_I2C_EDGES 20
#define MIN_SPI_EDGES 20

static volatile edge_timing_t edge_buffer[MAX_EDGES];
static volatile uint32_t edge_count = 0;
static volatile bool measurement_active = false;

// Pin configurations
static struct {
    uint uart_pin;
    uint i2c_scl;
    uint i2c_sda;
    uint spi_sck;
    uint spi_mosi;
    uint spi_miso;
} pins;

// Standard UART baud rates
static const uint32_t STANDARD_BAUDS[] = {
    300, 1200, 2400, 4800, 9600, 19200, 
    38400, 57600, 74880, 115200};

// GPIO interrupt handler
static void gpio_callback(uint gpio, uint32_t events) {
    if (!measurement_active || edge_count >= MAX_EDGES) return;
    
    edge_timing_t edge;
    edge.timestamp = time_us_32();
    edge.level = gpio_get(gpio);
    
    edge_buffer[edge_count++] = edge;
}

void protocol_analyzer_init(uint uart_pin, uint i2c_scl, uint i2c_sda, 
                          uint spi_sck, uint spi_mosi, uint spi_miso) {
    // Store pin configurations
    pins.uart_pin = uart_pin;
    pins.i2c_scl = i2c_scl;
    pins.i2c_sda = i2c_sda;
    pins.spi_sck = spi_sck;
    pins.spi_mosi = spi_mosi;
    pins.spi_miso = spi_miso;
    
    // Initialize UART pin
    gpio_init(uart_pin);
    gpio_set_dir(uart_pin, GPIO_IN);
    gpio_pull_up(uart_pin);
    gpio_set_irq_enabled_with_callback(uart_pin, 
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, 
                                     &gpio_callback);
    
    // Initialize I2C pins (pulled high by default)
    gpio_init(i2c_scl);
    gpio_init(i2c_sda);
    gpio_set_dir(i2c_scl, GPIO_IN);
    gpio_set_dir(i2c_sda, GPIO_IN);
    gpio_pull_up(i2c_scl);
    gpio_pull_up(i2c_sda);
    gpio_set_irq_enabled(i2c_scl, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(i2c_sda, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    
    // Initialize SPI pins
    gpio_init(spi_sck);
    gpio_init(spi_mosi);
    gpio_init(spi_miso);
    gpio_set_dir(spi_sck, GPIO_IN);
    gpio_set_dir(spi_mosi, GPIO_IN);
    gpio_set_dir(spi_miso, GPIO_IN);
    gpio_set_irq_enabled(spi_sck, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    
    printf("Protocol analyzer initialized\n");
}

static bool analyze_uart_timing(const edge_timing_t* edges, uint32_t count, uart_params_t* params) {
    uint32_t min_interval = UINT32_MAX;
    uint32_t max_interval = 0;
    
    // Calculate intervals between edges
    for (int i = 1; i < count; i++) {
        uint32_t interval = edges[i].timestamp - edges[i-1].timestamp;
        if (interval > 5 && interval < 1000000) {
            if (interval < min_interval) min_interval = interval;
            if (interval > max_interval) max_interval = interval;
        }
    }
    
    // Calculate approximate baud rate
    uint32_t raw_baud = 1000000 / min_interval;
    
    // Find closest standard baud rate
    uint32_t closest_baud = 9600;
    float min_error = 100.0f;
    
    for (int i = 0; i < sizeof(STANDARD_BAUDS)/sizeof(STANDARD_BAUDS[0]); i++) {
        float error = fabs((float)raw_baud - (float)STANDARD_BAUDS[i]) / 
                     (float)STANDARD_BAUDS[i] * 100.0f;
        if (error < min_error) {
            min_error = error;
            closest_baud = STANDARD_BAUDS[i];
        }
    }
    
    params->baud_rate = closest_baud;
    params->bit_time_us = 1000000 / closest_baud;
    params->base.error_margin = min_error;
    params->base.is_valid = min_error < 5.0f;
    params->base.sample_count = count;
    
    return params->base.is_valid;
}

protocol_result_t analyze_protocol(uint32_t timeout_ms) {
    protocol_result_t result = {0};
    edge_count = 0;
    measurement_active = true;
    
    // Wait for edges or timeout
    uint32_t start_time = time_us_32();
    while (edge_count < MIN_EDGES_FOR_VALID && 
           (time_us_32() - start_time) < (timeout_ms * 1000)) {
        tight_loop_contents();
    }
    
    measurement_active = false;
    
    if (edge_count < MIN_EDGES_FOR_VALID) {
        result.detected_protocol = PROTOCOL_UNKNOWN;
        snprintf(result.error_message, sizeof(result.error_message),
                "Insufficient edges detected (%lu/%d)", edge_count, MIN_EDGES_FOR_VALID);
        return result;
    }
    
    // Try UART analysis first
    uart_params_t uart_params = {
        .base.type = PROTOCOL_UART
    };
    
    if (analyze_uart_timing((edge_timing_t*)edge_buffer, edge_count, &uart_params)) {
        result.detected_protocol = PROTOCOL_UART;
        result.params.uart = uart_params;
        result.is_valid = true;
    }
    // Add I2C and SPI detection here when implemented
    
    return result;
}

void display_protocol_results(const protocol_result_t* result) {
    printf("\n=== Protocol Analysis Results ===\n");
    
    if (!result->is_valid) {
        printf("✗ Protocol detection failed\n");
        printf("Error: %s\n", result->error_message);
        printf("==============================\n");
        return;
    }
    
    printf("✓ Protocol Detected: %s\n", get_protocol_name(result->detected_protocol));
    
    switch (result->detected_protocol) {
        case PROTOCOL_UART:
            printf("Baud Rate: %lu\n", result->params.uart.baud_rate);
            printf("Bit Time: %lu µs\n", result->params.uart.bit_time_us);
            printf("Error Margin: %.1f%%\n", result->params.uart.base.error_margin);
            printf("Sample Count: %lu\n", result->params.uart.base.sample_count);
            break;
            
        case PROTOCOL_I2C:
            // Add I2C specific display
            break;
            
        case PROTOCOL_SPI:
            // Add SPI specific display
            break;
            
        default:
            printf("Unknown protocol parameters\n");
            break;
    }
    
    printf("==============================\n");
}

const char* get_protocol_name(protocol_type_t type) {
    switch (type) {
        case PROTOCOL_UART: return "UART";
        case PROTOCOL_I2C:  return "I2C";
        case PROTOCOL_SPI:  return "SPI";
        default:           return "Unknown";
    }
}