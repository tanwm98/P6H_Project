// // signal_generator.c
// #include "test.h"

// // Shared state variables
// static volatile struct {
//     uint32_t timestamps[4][MAX_EDGES];  // For UART, SDA, SCL, SCK
//     uint32_t edge_counts[4];
//     protocol_type_t detected_protocol;
//     bool detection_active;
//     critical_section_t cs;
//     alarm_pool_t* alarm_pool;
//     alarm_id_t timeout_alarm;
// } detector_state;

// // Forward declarations of internal functions
// static void reset_detection_state(void);
// static void process_edge_detection(uint gpio, uint32_t timestamp);
// static bool analyze_uart_pattern(void);
// static bool analyze_i2c_pattern(void);
// static bool analyze_spi_pattern(void);
// static void calculate_timing_info(protocol_result_t* result);

// // GPIO interrupt handler
// static void gpio_callback(uint gpio, uint32_t events) {
//     if (!detector_state.detection_active) return;
    
//     uint32_t timestamp = time_us_32();
//     int index = -1;
    
//     // Map GPIO to array index
//     if (gpio == UART_MONITOR_PIN) index = 0;
//     else if (gpio == I2C_SDA_PIN) index = 1;
//     else if (gpio == I2C_SCL_PIN) index = 2;
//     else if (gpio == SPI_SCK_PIN) index = 3;
    
//     if (index >= 0) {
//         critical_section_enter_blocking(&detector_state.cs);
//         if (detector_state.edge_counts[index] < MAX_EDGES) {
//             detector_state.timestamps[index][detector_state.edge_counts[index]++] = timestamp;
//             process_edge_detection(gpio, timestamp);
//         }
//         critical_section_exit(&detector_state.cs);
//     }
// }

// static void process_edge_detection(uint gpio, uint32_t timestamp) {
//     // Only process if we have enough edges
//     if (detector_state.edge_counts[0] < MIN_EDGES_FOR_DETECTION) return;
    
//     // Check for protocol patterns
//     if (detector_state.detected_protocol == PROTOCOL_UNKNOWN) {
//         if (analyze_uart_pattern()) {
//             detector_state.detected_protocol = PROTOCOL_UART;
//         } else if (analyze_i2c_pattern()) {
//             detector_state.detected_protocol = PROTOCOL_I2C;
//         } else if (analyze_spi_pattern()) {
//             detector_state.detected_protocol = PROTOCOL_SPI;
//         }
//     }
// }

// static bool analyze_uart_pattern(void) {
//     if (detector_state.edge_counts[0] < MIN_EDGES_FOR_DETECTION) return false;
    
//     // Look for consistent bit times indicating UART
//     uint32_t min_interval = UINT32_MAX;
//     uint32_t max_interval = 0;
    
//     for (int i = 1; i < detector_state.edge_counts[0]; i++) {
//         uint32_t interval = detector_state.timestamps[0][i] - 
//                            detector_state.timestamps[0][i-1];
//         if (interval > 5 && interval < 1000000) {  // Filter noise
//             if (interval < min_interval) min_interval = interval;
//             if (interval > max_interval) max_interval = interval;
//         }
//     }
    
//     // Check if intervals are consistent with UART timing
//     return (max_interval < min_interval * 3) && (min_interval > 8);  // >8μs = <115200 baud
// }

// static bool analyze_i2c_pattern(void) {
//     if (detector_state.edge_counts[1] < MIN_EDGES_FOR_DETECTION || 
//         detector_state.edge_counts[2] < MIN_EDGES_FOR_DETECTION) return false;
    
//     // Look for SDA transitions while SCL is high (start/stop conditions)
//     for (int i = 1; i < detector_state.edge_counts[1]; i++) {
//         uint32_t sda_transition = detector_state.timestamps[1][i];
//         bool scl_high = false;
        
//         // Find SCL state at SDA transition
//         for (int j = 0; j < detector_state.edge_counts[2]; j++) {
//             if (detector_state.timestamps[2][j] < sda_transition) {
//                 scl_high = !scl_high;
//             } else {
//                 break;
//             }
//         }
        
//         if (scl_high) return true;  // Found start/stop condition
//     }
//     return false;
// }

// static bool analyze_spi_pattern(void) {
//     if (detector_state.edge_counts[3] < MIN_EDGES_FOR_DETECTION) return false;
    
//     // Look for regular clock pattern and CS assertion
//     uint32_t first_interval = detector_state.timestamps[3][1] - 
//                              detector_state.timestamps[3][0];
//     int consistent_intervals = 0;
    
//     for (int i = 2; i < detector_state.edge_counts[3]; i++) {
//         uint32_t interval = detector_state.timestamps[3][i] - 
//                            detector_state.timestamps[3][i-1];
//         if (abs(interval - first_interval) < (first_interval / 4)) {
//             consistent_intervals++;
//         }
//     }
    
//     return consistent_intervals >= (MIN_EDGES_FOR_DETECTION - 2);
// }

// void signal_generator_init(void) {
//     // Initialize output pin
//     gpio_init(OUTPUT_PIN);
//     gpio_set_dir(OUTPUT_PIN, GPIO_OUT);
//     gpio_put(OUTPUT_PIN, 0);
    
//     protocol_detector_init();
//     printf("Signal generator initialized\n");
// }

// void protocol_detector_init(void) {
//     // Initialize critical section
//     critical_section_init(&detector_state.cs);
    
//     // Initialize all pins
//     gpio_init(UART_MONITOR_PIN);
//     gpio_init(I2C_SDA_PIN);
//     gpio_init(I2C_SCL_PIN);
//     gpio_init(SPI_SCK_PIN);
//     gpio_init(SPI_MOSI_PIN);
//     gpio_init(SPI_MISO_PIN);
//     gpio_init(SPI_CS_PIN);
    
//     // Set all as inputs
//     gpio_set_dir(UART_MONITOR_PIN, GPIO_IN);
//     gpio_set_dir(I2C_SDA_PIN, GPIO_IN);
//     gpio_set_dir(I2C_SCL_PIN, GPIO_IN);
//     gpio_set_dir(SPI_SCK_PIN, GPIO_IN);
//     gpio_set_dir(SPI_MOSI_PIN, GPIO_IN);
//     gpio_set_dir(SPI_MISO_PIN, GPIO_IN);
//     gpio_set_dir(SPI_CS_PIN, GPIO_IN);
    
//     // Enable interrupts with callback
//     gpio_set_irq_enabled_with_callback(UART_MONITOR_PIN, 
//                                      GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
//                                      true, &gpio_callback);
//     gpio_set_irq_enabled(I2C_SDA_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
//     gpio_set_irq_enabled(I2C_SCL_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
//     gpio_set_irq_enabled(SPI_SCK_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    
//     reset_detection_state();
//     printf("Protocol detector initialized\n");
// }

// static void reset_detection_state(void) {
//     critical_section_enter_blocking(&detector_state.cs);
//     memset((void*)detector_state.edge_counts, 0, sizeof(detector_state.edge_counts));
//     detector_state.detected_protocol = PROTOCOL_UNKNOWN;
//     detector_state.detection_active = false;
//     critical_section_exit(&detector_state.cs);
// }

// protocol_result_t detect_protocol(uint32_t timeout_ms) {
//     protocol_result_t result = {0};
//     reset_detection_state();
    
//     // Start detection
//     critical_section_enter_blocking(&detector_state.cs);
//     detector_state.detection_active = true;
//     critical_section_exit(&detector_state.cs);
    
//     // Wait for detection or timeout
//     uint32_t start_time = time_us_32();
//     while ((time_us_32() - start_time) < (timeout_ms * 1000)) {
//         if (detector_state.detected_protocol != PROTOCOL_UNKNOWN) {
//             result.type = detector_state.detected_protocol;
//             result.signal_detected = true;
//             break;
//         }
//         tight_loop_contents();
//     }
    
//     // Process results if signal detected
//     if (result.signal_detected) {
//         calculate_timing_info(&result);
        
//         // Set protocol-specific configurations
//         switch (result.type) {
//             case PROTOCOL_UART:
//                 result.config.uart.baud_rate = result.timing.frequency;
//                 result.config.uart.data_bits = 8;  // Default
//                 result.config.uart.stop_bits = 1;  // Default
//                 break;
                
//             case PROTOCOL_I2C:
//                 result.config.i2c.address_detected = false;  // Would need more analysis
//                 result.config.i2c.is_10bit = false;
//                 break;
                
//             case PROTOCOL_SPI:
//                 result.config.spi.mode = 0;  // Would need more analysis
//                 result.config.spi.cs_active_low = true;
//                 result.config.spi.msb_first = true;
//                 break;
                
//             default:
//                 break;
//         }
//     }
    
//     cleanup_detection();
//     return result;
// }

// static void calculate_timing_info(protocol_result_t* result) {
//     uint32_t total_period = 0;
//     uint32_t edge_count = 0;
//     float max_deviation = 0;
    
//     switch (result->type) {
//         case PROTOCOL_UART:
//             edge_count = detector_state.edge_counts[0];
//             if (edge_count > 1) {
//                 uint32_t min_interval = UINT32_MAX;
//                 for (int i = 1; i < edge_count; i++) {
//                     uint32_t interval = detector_state.timestamps[0][i] - 
//                                       detector_state.timestamps[0][i-1];
//                     if (interval < min_interval && interval > 5) {
//                         min_interval = interval;
//                     }
//                 }
//                 result->timing.frequency = 1000000 / min_interval;
//             }
//             break;
            
//         case PROTOCOL_I2C:
//             edge_count = detector_state.edge_counts[2];  // Use SCL
//             if (edge_count > 1) {
//                 total_period = detector_state.timestamps[2][edge_count-1] - 
//                               detector_state.timestamps[2][0];
//                 result->timing.frequency = 1000000 / (total_period / edge_count / 2);
//             }
//             break;
            
//         case PROTOCOL_SPI:
//             edge_count = detector_state.edge_counts[3];  // Use SCK
//             if (edge_count > 1) {
//                 total_period = detector_state.timestamps[3][edge_count-1] - 
//                               detector_state.timestamps[3][0];
//                 result->timing.frequency = 1000000 / (total_period / edge_count / 2);
//             }
//             break;
            
//         default:
//             result->timing.frequency = 0;
//             break;
//     }
    
//     // Calculate error margin
//     if (edge_count > 2) {
//         uint32_t expected_period = 1000000 / result->timing.frequency;
//         for (int i = 1; i < edge_count; i++) {
//             uint32_t actual_period;
//             switch (result->type) {
//                 case PROTOCOL_UART:
//                     actual_period = detector_state.timestamps[0][i] - 
//                                   detector_state.timestamps[0][i-1];
//                     break;
//                 case PROTOCOL_I2C:
//                     actual_period = detector_state.timestamps[2][i] - 
//                                   detector_state.timestamps[2][i-1];
//                     break;
//                 case PROTOCOL_SPI:
//                     actual_period = detector_state.timestamps[3][i] - 
//                                   detector_state.timestamps[3][i-1];
//                     break;
//                 default:
//                     continue;
//             }
//             float deviation = abs(actual_period - expected_period) * 100.0f / expected_period;
//             if (deviation > max_deviation) {
//                 max_deviation = deviation;
//             }
//         }
//         result->timing.error_margin = max_deviation;
//     }
    
//     result->timing.is_valid = (result->timing.frequency > 0 && 
//                               result->timing.error_margin < MAX_TIMING_ERROR_PCT);
// }

// void cleanup_detection(void) {
//     critical_section_enter_blocking(&detector_state.cs);
//     detector_state.detection_active = false;
//     critical_section_exit(&detector_state.cs);
// }

// void abort_detection(void) {
//     cleanup_detection();
//     reset_detection_state();
// }

// signal_status_t reproduce_sequence(const sequence_data_t* sequence) {
//     if (!sequence || sequence->count != 10) {
//         return SG_ERROR_INVALID_DATA;
//     }
    
//     // Temporarily disable protocol detection
//     bool was_active = detector_state.detection_active;
//     detector_state.detection_active = false;
    
//     signal_status_t final_status = SG_SUCCESS;
    
//     // Run sequence twice as per requirements
//     for (int replay = 0; replay < 2; replay++) {
//         printf("\nStarting replay %d/2\n", replay + 1);
//         uint64_t start_time = time_us_64();
//         uint32_t timing_errors = 0;
        
//         // Generate each pulse
//         for (int i = 0; i < sequence->count; i++) {
//             // Wait for correct timestamp
//             while (time_us_64() - start_time < sequence->pulses[i].timestamp) {
//                 tight_loop_contents();
//             }
            
//             // Generate pulse
//             gpio_put(OUTPUT_PIN, sequence->pulses[i].level);
            
//             // Verify timing
//             uint64_t actual_time = time_us_64() - start_time;
//             int64_t timing_error = actual_time - sequence->pulses[i].timestamp;
            
//             printf("Pulse %d: Level=%d, Time=%lu us, Error=%lld us\n",
//                    i + 1, sequence->pulses[i].level, 
//                    (uint32_t)actual_time, timing_error);
                   
//             if (abs(timing_error) > TIMING_ERROR_THRESHOLD) {
//                 timing_errors++;
//             }
//         }
        
//         // Reset output between replays
//         gpio_put(OUTPUT_PIN, 0);
        
//         // Check timing accuracy
//         if (timing_errors > 2) {
//             final_status = SG_ERROR_TIMING;
//         }
        
//         // Inter-replay delay
//         sleep_ms(100);
//     }
    
//     // Restore detection state
//     detector_state.detection_active = was_active;
//     return final_status;
// }

// void display_protocol_results(const protocol_result_t* result) {
//     printf("\n=== Protocol Detection Results ===\n");
    
//     if (!result->signal_detected) {
//         printf("✗ No valid protocol detected\n");
//         return;
//     }
    
//     switch (result->type) {
//         case PROTOCOL_UART:
//             printf("✓ UART Protocol Detected\n");
//             printf("Baud Rate: %lu\n", result->timing.frequency);
//             printf("Data Bits: %d\n", result->config.uart.data_bits);
//             printf("Stop Bits: %d\n", result->config.uart.stop_bits);
//             printf("Parity: %s\n", result->config.uart.parity_enabled ? 
//                    (result->config.uart.parity_even ? "Even" : "Odd") : "None");
//             break;
            
//         case PROTOCOL_I2C:
//             printf("✓ I2C Protocol Detected\n");
//             printf("Clock Frequency: %lu Hz\n", result->timing.frequency);
//             if (result->config.i2c.address_detected) {
//                 printf("Address: 0x%02X (%s)\n", result->config.i2c.address,
//                        result->config.i2c.is_10bit ? "10-bit" : "7-bit");
//             }
//             break;
            
//         case PROTOCOL_SPI:
//             printf("✓ SPI Protocol Detected\n");
//             printf("Clock Frequency: %lu Hz\n", result->timing.frequency);
//             printf("Mode: %d\n", result->config.spi.mode);
//             printf("CS Active: %s\n", result->config.spi.cs_active_low ? "Low" : "High");
//             printf("Bit Order: %s First\n", result->config.spi.msb_first ? "MSB" : "LSB");
//             break;
            
//         default:
//             printf("✗ Unknown Protocol\n");
//             break;
//     }
    
//     printf("Error Margin: %.1f%%\n", result->timing.error_margin);
//     printf("Signal Quality: %s\n", 
//            result->timing.is_valid ? "Good" : "Poor");
//     printf("=============================\n");
// }

// const char* get_status_string(signal_status_t status) {
//     switch (status) {
//         case SG_SUCCESS:
//             return "Success";
//         case SG_ERROR_INVALID_DATA:
//             return "Invalid Data";
//         case SG_ERROR_TIMING:
//             return "Timing Error";
//         case SG_ERROR_NO_SIGNAL:
//             return "No Signal Detected";
//         case SG_ERROR_PROTOCOL_UNKNOWN:
//             return "Unknown Protocol";
//         default:
//             return "Unknown Error";
//     }
// }