// #include "uart.h"
// #include "protocol_analyzer.h"

// // Standard UART baud rates
// const uint32_t STANDARD_BAUDS[] = {
//     300, 1200, 2400, 4800, 9600, 19200, 
//     38400, 57600, 74880, 115200, 230400, 460800
// };

// // Helper function to find closest standard baud rate
// static uint32_t find_closest_baud(uint32_t measured_baud) {
//     uint32_t closest_baud = 9600; // Default fallback
//     float min_error = 100.0f;
    
//     for (int i = 0; i < sizeof(STANDARD_BAUDS)/sizeof(STANDARD_BAUDS[0]); i++) {
//         float error = fabsf((float)measured_baud - (float)STANDARD_BAUDS[i]) / 
//                      (float)STANDARD_BAUDS[i] * 100.0f;
//         if (error < min_error) {
//             min_error = error;
//             closest_baud = STANDARD_BAUDS[i];
//         }
//     }
//     return closest_baud;
// }

// void uart_analyzer_init(void) {
//     gpio_init(UART_RX_PIN);
//     gpio_set_dir(UART_RX_PIN, GPIO_IN);
//     gpio_pull_down(UART_RX_PIN); // Enable pull-up for UART RX
//     printf("UART RX Pin Initial State: %d\n", gpio_get(UART_RX_PIN));
// }

// bool uart_analyze_timing(UARTEdgeTiming* edge_buffer, 
//                         uint32_t edge_count,
//                         UARTMetrics* metrics) {
//     if (!edge_buffer || !metrics || edge_count < MIN_UART_EDGES) {
//         return false;
//     }

//     // Initialize metrics
//     metrics->parity_errors = 0;
//     metrics->frame_errors = 0;
//     metrics->data_bits = 8;  // Default to 8 bits
//     metrics->stop_bits = 1;  // Default to 1 stop bit
//     metrics->parity_enabled = false;
//     metrics->parity_type = 'N';

//     uint32_t min_interval = UINT32_MAX;
//     uint32_t max_interval = 0;
//     uint32_t total_intervals = 0;
//     uint32_t interval_count = 0;
    
//     // First pass: Calculate base timing statistics
//     for (int i = 1; i < edge_count; i++) {
//         uint32_t interval = edge_buffer[i].timestamp - edge_buffer[i-1].timestamp;
        
//         // Filter out unreasonable intervals (noise)
//         if (interval > 5 && interval < 1000000) {
//             if (interval < min_interval) min_interval = interval;
//             if (interval > max_interval) max_interval = interval;
//             total_intervals += interval;
//             interval_count++;
//         }
//     }
    
//     if (interval_count == 0) return false;
    
//     // Calculate average interval and approximate baud rate
//     uint32_t avg_interval = total_intervals / interval_count;
//     uint32_t raw_baud = 1000000 / min_interval;
    
//     // Find closest standard baud rate
//     uint32_t detected_baud = find_closest_baud(raw_baud);
//     uint32_t expected_bit_time = uart_get_bit_period(detected_baud);
    
//     // Second pass: Analyze frame structure and errors
//     uint32_t frame_count = 0;
//     uint32_t valid_frames = 0;
//     uint8_t current_byte = 0;
//     bool in_frame = false;
//     int bit_count = 0;
    
//     for (int i = 1; i < edge_count; i++) {
//         uint32_t interval = edge_buffer[i].timestamp - edge_buffer[i-1].timestamp;
//         float interval_error = uart_calculate_timing_error(interval, expected_bit_time);
        
//         // Start bit detection (high to low transition with correct timing)
//         if (!in_frame && edge_buffer[i-1].level && !edge_buffer[i].level && 
//             interval_error < MAX_UART_ERROR) {
//             in_frame = true;
//             bit_count = 0;
//             current_byte = 0;
//             frame_count++;
//             continue;
//         }
        
//         // Data bits and stop bit processing
//         if (in_frame) {
//             bit_count++;
            
//             if (bit_count <= 8) {  // Data bits
//                 current_byte = (current_byte >> 1) | (edge_buffer[i].level ? 0x80 : 0);
//             }
//             else if (bit_count == 9) {  // Stop bit
//                 if (edge_buffer[i].level) {  // Valid stop bit
//                     valid_frames++;
//                 } else {
//                     metrics->frame_errors++;
//                 }
//                 in_frame = false;
//             }
//         }
//     }
    
//     // Calculate final metrics
//     if (frame_count > 0) {
//         float frame_error_rate = (float)metrics->frame_errors / frame_count * 100.0f;
//         float timing_error = fabsf((float)raw_baud - (float)detected_baud) / 
//                            (float)detected_baud * 100.0f;
        
//         metrics->baud_rate = detected_baud;
//         metrics->error_margin = timing_error;
//         metrics->sample_count = frame_count;
//         metrics->is_valid = (timing_error < MAX_UART_ERROR) && 
//                           (frame_error_rate < MAX_UART_ERROR);
        
//         printf("UART Analysis:\n");
//         printf("- Detected Baud: %lu\n", detected_baud);
//         printf("- Timing Error: %.1f%%\n", timing_error);
//         printf("- Frame Errors: %lu/%lu (%.1f%%)\n", 
//                metrics->frame_errors, frame_count, frame_error_rate);
//         printf("- Valid Frames: %lu\n", valid_frames);
//     } else {
//         metrics->is_valid = false;
//     }
    
//     return metrics->is_valid;
// }

// bool uart_check_parity(uint8_t data, bool parity_bit, char parity_type) {
//     if (parity_type == 'N') return true;  // No parity checking
    
//     // Count number of 1s in data
//     uint8_t ones = 0;
//     for (int i = 0; i < 8; i++) {
//         if (data & (1 << i)) ones++;
//     }
    
//     // Check parity
//     if (parity_type == 'E') {
//         return ((ones + (parity_bit ? 1 : 0)) % 2) == 0;
//     } else { // 'O'
//         return ((ones + (parity_bit ? 1 : 0)) % 2) == 1;
//     }
// }

// bool uart_detect_frame_error(UARTEdgeTiming* edges, uint32_t start_index) {
//     // Check for proper start bit
//     if (edges[start_index].level) return true;
    
//     // Check for proper stop bit
//     uint32_t stop_bit_index = start_index + 9;  // 1 start + 8 data bits
//     if (stop_bit_index >= MAX_EDGES || !edges[stop_bit_index].level) return true;
    
//     return false;
// }

// float uart_calculate_timing_error(uint32_t measured_interval, uint32_t expected_interval) {
//     return fabsf((float)measured_interval - (float)expected_interval) / 
//            (float)expected_interval * 100.0f;
// }

// const char* uart_get_validity_string(bool is_valid) {
//     return is_valid ? "Valid UART" : "Invalid or Unknown";
// }

// static struct {
//     uint32_t high_edges;
//     uint32_t low_edges;
//     uint32_t total_edges;
//     uint32_t last_level;
//     uint32_t missed_transitions;
//     uint32_t last_event_time;
//     uint32_t min_interval;
//     uint32_t max_interval;
// } uart_debug = {0};

// void uart_debug_edge(uint gpio, uint32_t events, uint32_t now) {
//     bool current_level = (events & GPIO_IRQ_EDGE_RISE) ? 1 : 0;  // Use event type
//     uint32_t interval = now - uart_debug.last_event_time;
    
//     // Detailed event logging
//     printf("Edge Event - Time: %lu, Events: 0x%lx, Level: %d\n", 
//            now, events, current_level);
    
//     // Count specific edge types
//     if (events & GPIO_IRQ_EDGE_RISE) {
//         uart_debug.high_edges++;
//         printf("  Rising Edge detected\n");
//     }
//     if (events & GPIO_IRQ_EDGE_FALL) {
//         uart_debug.low_edges++;
//         printf("  Falling Edge detected\n");
//     }
    
//     // Track timing
//     if (uart_debug.total_edges > 0) {
//         if (interval < uart_debug.min_interval || uart_debug.min_interval == 0) {
//             uart_debug.min_interval = interval;
//         }
//         if (interval > uart_debug.max_interval) {
//             uart_debug.max_interval = interval;
//         }
        
//         // Check for missed or unexpected transitions
//         if (current_level == uart_debug.last_level) {
//             uart_debug.missed_transitions++;
//             printf("  Warning: Same level transition detected!\n");
//         }
//     }
    
//     uart_debug.last_level = current_level;
//     uart_debug.last_event_time = now;
//     uart_debug.total_edges++;
    
//     // Print comprehensive stats every 20 edges
//     if (uart_debug.total_edges % 20 == 0) {
//         printf("\nUART Detailed Debug Stats:\n");
//         printf("High Edges: %lu\n", uart_debug.high_edges);
//         printf("Low Edges: %lu\n", uart_debug.low_edges);
//         printf("Total Edges: %lu\n", uart_debug.total_edges);
//         printf("Missed Transitions: %lu\n", uart_debug.missed_transitions);
//         printf("Current Level: %d\n", current_level);
//         printf("Min Interval: %lu us\n", uart_debug.min_interval);
//         printf("Max Interval: %lu us\n", uart_debug.max_interval);
//         printf("Expected bit time for 300 baud: %lu us\n", 1000000/300);
//     }
// }

// void uart_debug_reset(void) {
//     memset(&uart_debug, 0, sizeof(uart_debug));
//     printf("UART Debug Reset - Initial Pin State: %d\n", gpio_get(UART_RX_PIN));
// }

// // Add this function to test the pin directly
// void uart_test_pin(void) {
//     printf("\nUART Pin Test Starting...\n");
//     for(int i = 0; i < 100; i++) {
//         printf("UART RX Pin State: %d\n", gpio_get(UART_RX_PIN));
//         sleep_ms(100);  // Read every 100ms
//     }
// }