// signal_generator.c
#include "signal_generator.h"

static volatile uint32_t edge_timestamps[256];
static volatile uint32_t edge_count = 0;
static volatile bool analysis_complete = false;
static volatile bool sequence_running = false;

// Standard baud rates for validation
static const uint32_t STANDARD_BAUDS[] = {9600, 19200, 38400, 57600, 115200};
static const uint32_t NUM_STANDARD_BAUDS = sizeof(STANDARD_BAUDS) / sizeof(uint32_t);

// GPIO interrupt handler for UART analysis
static void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == UART_MONITOR_PIN && edge_count < 256) {
        edge_timestamps[edge_count++] = time_us_32();
    }
}

// Initialize hardware
void signal_generator_init(void) {
    // Initialize output pin
    gpio_init(OUTPUT_PIN);
    gpio_set_dir(OUTPUT_PIN, GPIO_OUT);
    gpio_put(OUTPUT_PIN, 0);
    
    // Initialize UART monitor pin
    gpio_init(UART_MONITOR_PIN);
    gpio_set_dir(UART_MONITOR_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(UART_MONITOR_PIN, 
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, 
                                     &gpio_callback);
    
    printf("Signal generator initialized (GP3=output, GP4=UART monitor)\n");
}

// Generate pulse sequence
signal_status_t reproduce_sequence(const sequence_data_t* sequence) {
    if (!sequence || sequence->count != 10) {
        return SG_ERROR_INVALID_DATA;
    }
    
    sequence_running = true;
    signal_status_t final_status = SG_SUCCESS;
    
    // Run sequence twice as per requirements
    for (int replay = 0; replay < 2 && sequence_running; replay++) {
        printf("\nStarting replay %d/2\n", replay + 1);
        
        uint64_t start_time = time_us_64();
        uint32_t timing_errors = 0;
        
        // Generate each pulse
        for (int i = 0; i < sequence->count && sequence_running; i++) {
            // Wait for correct timestamp
            while (time_us_64() - start_time < sequence->pulses[i].timestamp && sequence_running) {
                tight_loop_contents();
            }
            
            // Generate pulse
            gpio_put(OUTPUT_PIN, sequence->pulses[i].level);
            
            // Verify timing
            uint64_t actual_time = time_us_64() - start_time;
            int64_t timing_error = actual_time - sequence->pulses[i].timestamp;
            
            // Log pulse details
            printf("Pulse %d: Level=%d, Time=%lu us, Error=%lld us\n",
                   i + 1, sequence->pulses[i].level, 
                   (uint32_t)actual_time, timing_error);
            
            if (abs(timing_error) > 50) { // 50μs error threshold
                timing_errors++;
            }
        }
        
        // Reset output between replays
        gpio_put(OUTPUT_PIN, 0);
        
        // Check timing accuracy
        if (timing_errors > 2) { // Allow up to 2 timing errors
            final_status = SG_ERROR_TIMING;
        }
        
        // Inter-replay delay
        sleep_ms(100);
    }
    
    sequence_running = false;
    return final_status;
}

// Analyze UART signal
uart_result_t analyze_uart_signal(uint32_t timeout_ms) {
    uart_result_t result = {0};
    
    // Reset analysis state
    edge_count = 0;
    analysis_complete = false;
    
    // Wait for data with timeout
    uint32_t start_time = time_us_32();
    while (edge_count < 20 && 
           (time_us_32() - start_time) < (timeout_ms * 1000)) {
        tight_loop_contents();
    }
    
    if (edge_count < 20) {
        result.is_valid = false;
        return result;
    }
    
    // Calculate intervals
    uint32_t total_interval = 0;
    uint32_t valid_intervals = 0;
    uint32_t min_interval = UINT32_MAX;
    
    for (int i = 1; i < edge_count; i++) {
        uint32_t interval = edge_timestamps[i] - edge_timestamps[i-1];
        if (interval > 1 && interval < 1000) {
            total_interval += interval;
            valid_intervals++;
            min_interval = (interval < min_interval) ? interval : min_interval;
        }
    }
    
    if (valid_intervals < 10) {
        result.is_valid = false;
        return result;
    }
    
    // Calculate baud rate
    result.bit_time = min_interval;
    uint32_t detected_baud = 1000000 / result.bit_time;
    
    // Find closest standard baud rate
    uint32_t min_diff = UINT32_MAX;
    for (int i = 0; i < NUM_STANDARD_BAUDS; i++) {
        uint32_t diff = abs(detected_baud - STANDARD_BAUDS[i]);
        if (diff < min_diff) {
            min_diff = diff;
            result.baud_rate = STANDARD_BAUDS[i];
        }
    }
    
    // Calculate error margin
    result.error_margin = ((float)min_diff / result.baud_rate) * 100.0f;
    result.is_valid = result.error_margin < 5.0f;
    result.sample_count = edge_count;
    
    return result;
}


void abort_sequence(void) {
    sequence_running = false;
    gpio_put(OUTPUT_PIN, 0);
}

#ifdef DEBUG_MODE
void print_uart_debug(const uart_result_t* result) {
    if (!result) return;
    
    printf("\n=== UART Analysis Debug ===\n");
    printf("Baud Rate: %lu\n", result->baud_rate);
    printf("Bit Time: %lu us\n", result->bit_time);
    printf("Error Margin: %.2f%%\n", result->error_margin);
    printf("Valid: %s\n", result->is_valid ? "Yes" : "No");
    printf("Samples: %lu\n", result->sample_count);
    printf("========================\n");
}
#endif