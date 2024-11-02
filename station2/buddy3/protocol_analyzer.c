#include "protocol_analyzer.h"
#include "uart.h"
#include "i2c.h"
#include "spi.h"

static volatile ProtocolMetrics protocol_metrics = {0};
static protocol_patterns_t patterns = {0};

void protocol_analyzer_init(void) {
    uart_analyzer_init();
    // i2c_analyzer_init();
    // spi_analyzer_init();
    
    protocol_metrics.is_capturing = false;
    protocol_metrics.edge_count = 0;
    protocol_metrics.detected_protocol = PROTOCOL_UNKNOWN;
    memset(&patterns, 0, sizeof(patterns));
}

static void detect_protocol_patterns(uint gpio, uint32_t events, uint32_t now) {
    static uint32_t last_edge_time = 0;
    static bool last_level = false;
    uint32_t interval = now - last_edge_time;
    bool current_level = (events & GPIO_IRQ_EDGE_RISE) ? 1 : 0;  // Use event type
    // UART start bit detection (high-to-low transition followed by 8-10 bits)
    if (last_level && !current_level) {
        patterns.uart_start_bits++;
    }
    
    // I2C START condition (SDA falling while SCL high)
    if (gpio == I2C_SDA_PIN && !current_level && gpio_get(I2C_SCL_PIN)) {
        patterns.i2c_start_conditions++;
    }
    
    // SPI clock edge counting
    if (gpio == SPI_SCK_PIN && (events & GPIO_IRQ_EDGE_RISE)) {
        patterns.spi_clock_edges++;
    }
    
    last_edge_time = now;
    last_level = current_level;
}

void handle_protocol_edge(uint gpio, uint32_t events, uint32_t now) {
    if (!protocol_metrics.is_capturing || protocol_metrics.edge_count >= MAX_EDGES) return;
    edge_timing_t edge;
    edge.timestamp = now;
    edge.level = (events & GPIO_IRQ_EDGE_RISE) ? 1 : 0;
    
    protocol_metrics.edge_buffer[protocol_metrics.edge_count++] = edge;
    
    // Detect protocol patterns in real-time
    detect_protocol_patterns(gpio, events, now);
    
    // Auto-identify protocol based on patterns
    identify_protocol();
    
    // If protocol identified, analyze specific protocol metrics
    if (protocol_metrics.detected_protocol != PROTOCOL_UNKNOWN) {
        analyze_protocol_data();
    }
}

static void identify_protocol(void) {
    // Simple threshold-based protocol identification
    if (patterns.uart_start_bits > 1) {
        protocol_metrics.detected_protocol = PROTOCOL_UART;
    } else if (patterns.i2c_start_conditions > 2) {
        protocol_metrics.detected_protocol = PROTOCOL_I2C;
    } else if (patterns.spi_clock_edges > 8) {
        protocol_metrics.detected_protocol = PROTOCOL_SPI;
    }
}

static void analyze_protocol_data(void) {
    switch (protocol_metrics.detected_protocol) {
        case PROTOCOL_UART:
            analyze_uart_data();
            break;
        case PROTOCOL_I2C:
            //analyze_i2c_data();
            break;
        case PROTOCOL_SPI:
            //analyze_spi_data();
            break;
        default:
            break;
    }
}

static void analyze_uart_data(void) {
    UARTEdgeTiming uart_edges[MAX_EDGES];
    UARTMetrics uart_metrics;
    
    // Convert edge buffer to UART timing
    for (int i = 0; i < protocol_metrics.edge_count; i++) {
        uart_edges[i].timestamp = protocol_metrics.edge_buffer[i].timestamp;
        uart_edges[i].level = protocol_metrics.edge_buffer[i].level;
    }
    
    if (uart_analyze_timing(uart_edges, protocol_metrics.edge_count, &uart_metrics)) {
        protocol_metrics.is_valid = true;
        protocol_metrics.baud_rate = uart_metrics.baud_rate;
        protocol_metrics.error_margin = uart_metrics.error_margin;
        protocol_metrics.sample_count = uart_metrics.sample_count;
        
        printf("UART detected - Baud: %lu, Error: %.1f%%, Parity Errors: %lu\n", 
               protocol_metrics.baud_rate, 
               protocol_metrics.error_margin,
               patterns.parity_errors);
    }
}

void start_protocol_capture(void) {
    uart_debug_reset();
    // Reset all metrics and patterns
    protocol_metrics.edge_count = 0;
    protocol_metrics.detected_protocol = PROTOCOL_UNKNOWN;
    protocol_metrics.is_valid = false;
    protocol_metrics.baud_rate = 0;
    protocol_metrics.error_margin = 0;
    memset(&patterns, 0, sizeof(patterns));
    
    protocol_metrics.is_capturing = true;
    printf("Starting protocol capture and analysis...\n");
}

void stop_protocol_capture(void) {
    protocol_metrics.is_capturing = false;
    
    // Print final analysis results
    printf("\nProtocol Analysis Results:\n");
    printf("Detected Protocol: %s\n", get_protocol_name(protocol_metrics.detected_protocol));
    
    switch (protocol_metrics.detected_protocol) {
        case PROTOCOL_UART:
            printf("UART Metrics:\n");
            printf("- Baud Rate: %lu\n", protocol_metrics.baud_rate);
            printf("- Error Margin: %.1f%%\n", protocol_metrics.error_margin);
            printf("- Parity Errors: %lu\n", patterns.parity_errors);
            printf("- Frame Errors: %lu\n", patterns.frame_errors);
            break;
            
        case PROTOCOL_I2C:
            printf("I2C Metrics:\n");
            printf("- Start Conditions: %lu\n", patterns.i2c_start_conditions);
            break;
            
        case PROTOCOL_SPI:
            printf("SPI Metrics:\n");
            printf("- Clock Transitions: %lu\n", patterns.spi_clock_edges);
            break;
            
        default:
            printf("No valid protocol detected\n");
    }
    
    // Reset state
    protocol_metrics.edge_count = 0;
    protocol_metrics.detected_protocol = PROTOCOL_UNKNOWN;
    protocol_metrics.is_valid = false;
}

const char* get_protocol_name(protocol_type_t type){
    switch (type) {
        case PROTOCOL_UART:
            return "UART";
        case PROTOCOL_I2C:
            return "I2C";
        case PROTOCOL_SPI:
            return "SPI";
        default:
            return "Unknown";
    }
}

bool is_protocol_capturing(void) {
    return protocol_metrics.is_capturing;
}

bool is_protocol_valid(void) {
    return protocol_metrics.is_valid;
}

float get_error_margin(void) {
    return protocol_metrics.error_margin;
}

uint32_t get_baud_rate(void) {
    return protocol_metrics.baud_rate;
}
protocol_type_t get_detected_protocol(void) {
    return protocol_metrics.detected_protocol;
}
uint32_t get_sample_count(void) {
    return protocol_metrics.sample_count;
}
uint32_t get_edge_count(void) {
    return protocol_metrics.edge_count;
}
void set_edge_count(uint32_t count) {
    protocol_metrics.edge_count = count;
}