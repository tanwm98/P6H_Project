#include "protocol_analyzer.h"

static volatile ProtocolMetrics protocol_metrics = {0};

// Standard UART baud rates
static const uint32_t STANDARD_BAUDS[] = {
    300, 1200, 2400, 4800, 9600, 19200, 
    38400, 57600, 74880, 115200
};

void protocol_analyzer_init(void) {
    // Initialize pins
    gpio_init(UART_RX_PIN);
    gpio_set_dir(UART_RX_PIN, GPIO_IN);
    gpio_pull_up(UART_RX_PIN);
    
    gpio_init(I2C_SCL_PIN);
    gpio_init(I2C_SDA_PIN);
    gpio_set_dir(I2C_SCL_PIN, GPIO_IN);
    gpio_set_dir(I2C_SDA_PIN, GPIO_IN);
    gpio_pull_up(I2C_SCL_PIN);
    gpio_pull_up(I2C_SDA_PIN);
    
    gpio_init(SPI_SCK_PIN);
    gpio_init(SPI_MOSI_PIN);
    gpio_init(SPI_MISO_PIN);
    gpio_set_dir(SPI_SCK_PIN, GPIO_IN);
    gpio_set_dir(SPI_MOSI_PIN, GPIO_IN);
    gpio_set_dir(SPI_MISO_PIN, GPIO_IN);
    
    // Initialize metrics
    protocol_metrics.is_capturing = false;
    protocol_metrics.edge_count = 0;
    protocol_metrics.detected_protocol = PROTOCOL_UNKNOWN;
}

void handle_protocol_edge(uint gpio, uint32_t events, uint32_t now) {
    if (!protocol_metrics.is_capturing || protocol_metrics.edge_count >= MAX_EDGES) return;
    
    edge_timing_t edge;
    edge.timestamp = now;
    edge.level = gpio_get(gpio);
    
    protocol_metrics.edge_buffer[protocol_metrics.edge_count++] = edge;
    
    // If we have enough edges, analyze the protocol
    if (protocol_metrics.edge_count >= MIN_EDGES_FOR_VALID) {
        analyze_captured_data();
    }
}

static void analyze_captured_data(void) {
    // Reset previous results
    protocol_metrics.is_valid = false;
    protocol_metrics.detected_protocol = PROTOCOL_UNKNOWN;
    protocol_metrics.baud_rate = 0;
    protocol_metrics.clock_freq = 0;
    protocol_metrics.error_margin = 0;
    
    // Try UART analysis first
    if (analyze_uart_timing()) {
        protocol_metrics.detected_protocol = PROTOCOL_UART;
        protocol_metrics.is_valid = true;
        printf("Protocol detected: UART at %lu baud\n", protocol_metrics.baud_rate);
    }
    // Add I2C and SPI detection here when implemented
}

static bool analyze_uart_timing(void) {
    uint32_t min_interval = UINT32_MAX;
    uint32_t max_interval = 0;
    
    // Calculate intervals between edges
    for (int i = 1; i < protocol_metrics.edge_count; i++) {
        uint32_t interval = protocol_metrics.edge_buffer[i].timestamp - 
                           protocol_metrics.edge_buffer[i-1].timestamp;
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
    
    protocol_metrics.baud_rate = closest_baud;
    protocol_metrics.error_margin = min_error;
    protocol_metrics.sample_count = protocol_metrics.edge_count;
    
    return min_error < 5.0f;
}

ProtocolMetrics get_protocol_metrics(void) {
    ProtocolMetrics metrics = protocol_metrics;
    return metrics;
}

bool is_protocol_capturing(void) {
    return protocol_metrics.is_capturing;
}

void start_protocol_capture(void) {
    protocol_metrics.is_capturing = true;
    protocol_metrics.edge_count = 0;
    protocol_metrics.detected_protocol = PROTOCOL_UNKNOWN;
    printf("Starting protocol capture...\n");
}

void stop_protocol_capture(void) {
    protocol_metrics.is_capturing = false;
    if (protocol_metrics.is_valid) {
        printf("Protocol Analysis Results:\n");
        printf("Protocol: %s\n", get_protocol_name(protocol_metrics.detected_protocol));
        if (protocol_metrics.detected_protocol == PROTOCOL_UART) {
            printf("Baud Rate: %lu\n", protocol_metrics.baud_rate);
            printf("Error Margin: %.1f%%\n", protocol_metrics.error_margin);
        }
    }
}

const char* get_protocol_name(protocol_type_t type) {
    switch (type) {
        case PROTOCOL_UART: return "UART";
        case PROTOCOL_I2C:  return "I2C";
        case PROTOCOL_SPI:  return "SPI";
        default:           return "Unknown";
    }
}