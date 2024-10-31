// #include "signal_generator.h"

// // Edge detection variables
// static volatile uint32_t edge_timestamps[256];
// static volatile uint32_t edge_count = 0;
// static volatile uint32_t edge_timestamps[256];
// static volatile bool sequence_running = false;


// pulse_data_t pulse_sequence[MAX_PULSES];
// int pulse_count = 0;

// void signal_generator_init(void) {
//     // Initialize output pin
//     gpio_init(OUTPUT_PIN);
//     gpio_set_dir(OUTPUT_PIN, GPIO_OUT);
//     gpio_put(OUTPUT_PIN, 0);
    
//     printf("Signal generator initialized (GP3=output, GP4=UART monitor)\n");
// }

// int get_pulse_count(void) {
//     return pulse_count;
// }

// // Function to parse a line from the stored sequence
// bool parse_pulse_line(const char* line, pulse_data_t* pulse) {
//     int timestamp;
//     int level;
    
//     if (sscanf(line, "%d,%d", &timestamp, &level) == 2) {
//         pulse->timestamp_ms = timestamp;
//         pulse->level = level ? true : false;
//         return true;
//     }
//     return false;
// }

// // bool read_stored_sequence() {
// //     const char* current_pos = stored_sequence;
// //     char line[32];
// //     int line_idx = 0;
// //     bool reading = false;
// //     pulse_count = 0;  // Reset pulse count
    
// //     printf("Reading stored pulse sequence...\n");
    
// //     while (*current_pos) {
// //         if (*current_pos == '\n') {
// //             line[line_idx] = '\0';
            
// //             if (strstr(line, "PULSE_SEQUENCE_START")) {
// //                 reading = true;
// //             }
// //             else if (strstr(line, "PULSE_SEQUENCE_END")) {
// //                 break;
// //             }
// //             else if (reading && !strstr(line, "timestamp_ms,level")) {
// //                 if (pulse_count < MAX_PULSES && parse_pulse_line(line, &pulse_sequence[pulse_count])) {
// //                     printf("Read pulse %d: %dms, level=%d\n", 
// //                            pulse_count + 1,
// //                            pulse_sequence[pulse_count].timestamp_ms,
// //                            pulse_sequence[pulse_count].level);
// //                     pulse_count++;
// //                 }
// //             }
            
// //             line_idx = 0;
// //         } else {
// //             if (line_idx < 31) {  // Prevent buffer overflow
// //                 line[line_idx++] = *current_pos;
// //             }
// //         }
// //         current_pos++;
// //     }

// //     printf("Total pulses read: %d\n", pulse_count);
// //     return pulse_count > 0;  // Return true if we read any pulses
// // }

// void replay_sequence() {
//     if (pulse_count == 0) {
//         printf("No pulses to replay!\n");
//         return;
//     }

//     printf("\nReplaying sequence twice...\n");
    
//     for (int replay = 0; replay < 2; replay++) {
//         printf("\nReplay %d/2:\n", replay + 1);
//         absolute_time_t start_time = get_absolute_time();
        
//         for (int i = 0; i < pulse_count; i++) {
//             busy_wait_until(delayed_by_ms(start_time, pulse_sequence[i].timestamp_ms));
//             gpio_put(OUTPUT_PIN, pulse_sequence[i].level);
            
//             printf("Pulse %d: Level=%d at %dms\n",
//                    i + 1,
//                    pulse_sequence[i].level,
//                    pulse_sequence[i].timestamp_ms);
//         }
        
//         gpio_put(OUTPUT_PIN, 0);
//         printf("Replay complete\n");
//         sleep_ms(1000);
//     }
// }

// // Find closest standard baud rate
// // static uint32_t get_closest_baud_rate(uint32_t measured_baud, float* error) {
// //     uint32_t closest_baud = 9600;  // Default fallback
// //     float min_error = 100.0f;
// //     for (int i = 0; i < sizeof(STANDARD_BAUDS)/sizeof(STANDARD_BAUDS[0]); i++) {
// //         float current_error = ((float)abs(measured_baud - STANDARD_BAUDS[i]) / STANDARD_BAUDS[i]) * 100.0f;
// //         if (current_error < min_error) {
// //             min_error = current_error;
// //             closest_baud = STANDARD_BAUDS[i];
// //         }
// //     }
// //     *error = min_error;
// //     return closest_baud;
// // }

// // uart_result_t analyze_uart_signal(uint32_t timeout_ms) {
// //     uart_result_t result = {0};
// //     printf("Starting UART analysis...\n");
// //     // Reset edge detection
// //     edge_count = 0;
// //     uint32_t start_time = time_us_32();

// //     // Wait for edges with timeout
// //     while (edge_count < 10 && 
// //            (time_us_32() - start_time) < (timeout_ms * 1000)) {
// //         tight_loop_contents();
// //     }
// //     if (edge_count < 10) {
// //         printf("Insufficient edges for analysis\n");
// //         return result;
// //     }

// //     // Calculate minimum bit time from intervals
// //     uint32_t min_interval = UINT32_MAX;
// //     for (int i = 1; i < edge_count; i++) {
// //         uint32_t interval = edge_timestamps[i] - edge_timestamps[i-1];
// //         // Filter obvious noise
// //         if (interval > 5 && interval < 1000000) {
// //             min_interval = (interval < min_interval) ? interval : min_interval;
// //         }
// //     }
// //     // Calculate raw baud rate
// //     uint32_t raw_baud = 1000000 / min_interval;
// //     printf("Raw measured baud: %lu\n", raw_baud);

// //     // Find closest standard baud rate
// //     result.baud_rate = get_closest_baud_rate(raw_baud, &result.error_margin);
// //     result.is_valid = result.error_margin < 10.0f;  // Allow up to 10% error

// //     // Print results
// //     printf("Analysis complete:\n");
// //     printf("Detected baud rate: %lu\n", result.baud_rate);
// //     printf("Error margin: %.1f%%\n", result.error_margin);
// //     return result;
// // }

// // void display_uart_results(const uart_result_t* result) {
// //     printf("\n=== UART Analysis Results ===\n");
// //     if (result->is_valid) {
// //         printf("✓ Baud Rate Detected: %lu\n", result->baud_rate);
// //         printf("Error Margin: %.1f%%\n", result->error_margin);
// //     } else {
// //         printf("✗ Baud Rate Detection Failed!\n");
// //         if (result->error_margin >= 10.0f) {
// //             printf("Non-standard baud rate detected\n");
// //             printf("Closest standard rate: %lu (%.1f%% error)\n", 
// //                    result->baud_rate, result->error_margin);
// //         }
// //     }
// //     printf("========================\n");
// // }
