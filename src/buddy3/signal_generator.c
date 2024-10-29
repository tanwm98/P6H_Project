#include "signal_generator.h"

// Edge detection variables
static volatile uint32_t edge_timestamps[256];
static volatile uint32_t edge_count = 0;
static volatile uint32_t edge_timestamps[256];
static volatile bool sequence_running = false;

// GPIO interrupt handler
static void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == UART_MONITOR_PIN && edge_count < 256) {
        // Store microsecond timestamp of edge
        edge_timestamps[edge_count++] = time_us_32();
    }
}

void signal_generator_init(void) {
    // Initialize output pin
    gpio_init(OUTPUT_PIN);
    gpio_set_dir(OUTPUT_PIN, GPIO_OUT);
    gpio_put(OUTPUT_PIN, 0);
    
    // Initialize UART monitor pin with interrupt
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

// Find closest standard baud rate
static uint32_t get_closest_baud_rate(uint32_t measured_baud, float* error) {
    uint32_t closest_baud = 9600;  // Default fallback
    float min_error = 100.0f;
    for (int i = 0; i < sizeof(STANDARD_BAUDS)/sizeof(STANDARD_BAUDS[0]); i++) {
        float current_error = ((float)abs(measured_baud - STANDARD_BAUDS[i]) / STANDARD_BAUDS[i]) * 100.0f;
        if (current_error < min_error) {
            min_error = current_error;
            closest_baud = STANDARD_BAUDS[i];
        }
    }
    *error = min_error;
    return closest_baud;
}

uart_result_t analyze_uart_signal(uint32_t timeout_ms) {
    uart_result_t result = {0};
    printf("Starting UART analysis...\n");
    // Reset edge detection
    edge_count = 0;
    uint32_t start_time = time_us_32();

    // Wait for edges with timeout
    while (edge_count < 10 && 
           (time_us_32() - start_time) < (timeout_ms * 1000)) {
        tight_loop_contents();
    }
    if (edge_count < 10) {
        printf("Insufficient edges for analysis\n");
        return result;
    }

    // Calculate minimum bit time from intervals
    uint32_t min_interval = UINT32_MAX;
    for (int i = 1; i < edge_count; i++) {
        uint32_t interval = edge_timestamps[i] - edge_timestamps[i-1];
        // Filter obvious noise
        if (interval > 5 && interval < 1000000) {
            min_interval = (interval < min_interval) ? interval : min_interval;
        }
    }
    // Calculate raw baud rate
    uint32_t raw_baud = 1000000 / min_interval;
    printf("Raw measured baud: %lu\n", raw_baud);

    // Find closest standard baud rate
    result.baud_rate = get_closest_baud_rate(raw_baud, &result.error_margin);
    result.is_valid = result.error_margin < 10.0f;  // Allow up to 10% error

    // Print results
    printf("Analysis complete:\n");
    printf("Detected baud rate: %lu\n", result.baud_rate);
    printf("Error margin: %.1f%%\n", result.error_margin);
    return result;
}

void display_uart_results(const uart_result_t* result) {
    printf("\n=== UART Analysis Results ===\n");
    if (result->is_valid) {
        printf("✓ Baud Rate Detected: %lu\n", result->baud_rate);
        printf("Error Margin: %.1f%%\n", result->error_margin);
    } else {
        printf("✗ Baud Rate Detection Failed!\n");
        if (result->error_margin >= 10.0f) {
            printf("Non-standard baud rate detected\n");
            printf("Closest standard rate: %lu (%.1f%% error)\n", 
                   result->baud_rate, result->error_margin);
        }
    }
    printf("========================\n");
}
