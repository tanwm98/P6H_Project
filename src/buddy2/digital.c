#include "digital.h"
#include "buddy1/sd_card.h"
#include "buddy5/wifi.h"
#include <string.h>

static PulseCapture capture = {0};
static void gpio_callback(uint gpio, uint32_t events);

void digital_init(void) {
    // Initialize input pin
    gpio_init(DIGITAL_INPUT_PIN);
    gpio_set_dir(DIGITAL_INPUT_PIN, GPIO_IN);
    gpio_pull_down(DIGITAL_INPUT_PIN);  // Pull-down to ensure clean low state

    // Initialize output pin
    gpio_init(DIGITAL_OUTPUT_PIN);
    gpio_set_dir(DIGITAL_OUTPUT_PIN, GPIO_OUT);
    gpio_put(DIGITAL_OUTPUT_PIN, 0);    // Start with output low

    // Setup GPIO interrupt for input pin
    gpio_set_irq_enabled_with_callback(DIGITAL_INPUT_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
        true, 
        &gpio_callback);

    printf("Digital pulse capture initialized on GP%d\n", DIGITAL_INPUT_PIN);
    printf("Digital pulse replay configured on GP%d\n", DIGITAL_OUTPUT_PIN);
}

static void gpio_callback(uint gpio, uint32_t events) {
    if (gpio != DIGITAL_INPUT_PIN || !capture.capturing) return;

    uint32_t current_time = time_us_32();
    bool is_rising = (events & GPIO_IRQ_EDGE_RISE) != 0;
    
    if ((is_rising && capture.expecting_high) || (!is_rising && !capture.expecting_high)) {
        if (capture.transition_count < MAX_TRANSITIONS) {
            // Store relative time from start
            uint32_t relative_time = current_time - capture.start_time;
            capture.transitions[capture.transition_count].time = relative_time;
            capture.transitions[capture.transition_count].state = is_rising;
            capture.transition_count++;
            capture.expecting_high = !capture.expecting_high;
            
            // If this isn't the first transition, calculate and print interval
            if (capture.transition_count > 1) {
                uint32_t interval = relative_time - 
                    capture.transitions[capture.transition_count-2].time;
                printf("Transition %d: %s at %lu us (interval: %lu us)\n", 
                    capture.transition_count,
                    is_rising ? "HIGH" : "LOW",
                    relative_time,
                    interval);
            } else {
                printf("Transition %d: %s at %lu us\n", 
                    capture.transition_count,
                    is_rising ? "HIGH" : "LOW",
                    relative_time);
            }
            
            if (capture.transition_count >= MAX_TRANSITIONS) {
                capture.capturing = false;
                printf("Capture complete: %d transitions captured\n", capture.transition_count);
            }
        }
    }
}

void start_pulse_capture(void) {
    // Reset capture state
    memset(&capture, 0, sizeof(capture));
    capture.capturing = true;
    capture.start_time = time_us_32();
    capture.expecting_high = true;  // Start expecting a rising edge
    printf("Starting pulse capture, waiting for rising edge...\n");
}

bool is_capture_complete(void) {
    return !capture.capturing && capture.transition_count == MAX_TRANSITIONS;
}

const Transition* get_captured_transitions(uint8_t* count) {
    if (count) {
        *count = capture.transition_count;
    }
    return capture.transitions;
}

void replay_pulses(uint8_t num_times) {
    if (capture.transition_count == 0) {
        printf("No transitions to replay\n");
        return;
    }

    printf("Replaying %d transitions %d times...\n", capture.transition_count, num_times);
    
    // Wait 2 seconds before starting replay
    sleep_ms(2000);
    
    for (int replay = 0; replay < num_times; replay++) {
        printf("Starting replay %d of %d\n", replay + 1, num_times);
        uint32_t start_time = time_us_32();
        uint32_t last_transition_time = start_time;
        
        // Reset output to initial state
        gpio_put(DIGITAL_OUTPUT_PIN, 0);
        
        for (int i = 0; i < capture.transition_count; i++) {
            uint32_t interval;
            if (i == 0) {
                interval = 1000; // 1ms initial delay
            } else {
                // Use the original captured interval
                interval = capture.transitions[i].time - capture.transitions[i-1].time;
            }
            
            // Calculate target time for this transition
            uint32_t target_time = last_transition_time + interval;
            
            // Wait precisely until the target time
            while (time_us_32() < target_time) {
                tight_loop_contents();
            }
            
            // Set output and record actual time
            gpio_put(DIGITAL_OUTPUT_PIN, capture.transitions[i].state);
            uint32_t current_time = time_us_32();
            uint32_t actual_interval = current_time - last_transition_time;
            last_transition_time = current_time;
            
            printf("Replay transition %2d: %4s at %lu us (interval: %lu us)\n",
                   i + 1,
                   capture.transitions[i].state ? "HIGH" : "LOW",
                   current_time - start_time,
                   actual_interval);
        }
        
        printf("Replay %d complete\n", replay + 1);
        
        if (replay < num_times - 1) {
            sleep_ms(100);  // Small delay between replays
        }
    }
    
    // Ensure we end in a low state
    gpio_put(DIGITAL_OUTPUT_PIN, 0);
    printf("All replays complete\n");
}

void save_pulses_to_file(const char* filename) {
    if (capture.transition_count == 0) {
        printf("No transitions to save\n");
        return;
    }

    // Get current timestamp
    uint32_t timestamp = get_timestamp();
    char timestamp_str[32];
    format_timestamp(timestamp, timestamp_str, sizeof(timestamp_str));
    
    // First time creation - add CSV header
    FIL file;
    FRESULT fr = f_open(&file, filename, FA_READ);
    bool need_header = (fr != FR_OK);
    f_close(&file);
    
    if (need_header) {
        char header[] = "timestamp,timestamp_readable,transition_number,state,time_us\n";
        writeDataToSD(filename, header, false);
    }

    // Save each transition with proper timestamp
    char buffer[256];
    for (int i = 0; i < capture.transition_count; i++) {
        snprintf(buffer, sizeof(buffer), "%lu,%s,%d,%d,%lu\n",
                timestamp,
                timestamp_str,
                i + 1,
                capture.transitions[i].state,
                capture.transitions[i].time);
        writeDataToSD(filename, buffer, true);
    }

    printf("Successfully saved capture at %s with %d transitions to %s\n", 
           timestamp_str, capture.transition_count, filename);
}

bool load_pulses_from_file(const char* filename) {
    FIL file;
    FRESULT fr;
    char line[256];
    uint32_t last_timestamp = 0;
    
    // Reset current capture state
    memset(&capture, 0, sizeof(capture));
    capture.start_time = time_us_32();
    
    // Open file for reading
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        printf("Failed to open file for reading: %s\n", filename);
        return false;
    }

    // Skip header line
    f_gets(line, sizeof(line), &file);
    
    // Read and store each transition
    uint32_t current_timestamp;
    char timestamp_str[64];
    int transition_num;
    int state;
    uint32_t time_us;

    while (f_gets(line, sizeof(line), &file)) {
        if (sscanf(line, "%lu,%[^,],%d,%d,%lu",
                   &current_timestamp,
                   timestamp_str,
                   &transition_num,
                   &state,
                   &time_us) == 5) {
            
            // Only process transitions from the latest timestamp
            if (current_timestamp >= last_timestamp) {
                if (current_timestamp > last_timestamp) {
                    // New sequence found, reset count
                    capture.transition_count = 0;
                    last_timestamp = current_timestamp;
                }
                
                if (capture.transition_count < MAX_TRANSITIONS) {
                    capture.transitions[capture.transition_count].time = capture.start_time + time_us;
                    capture.transitions[capture.transition_count].state = (bool)state;
                    capture.transition_count++;
                }
            }
        }
    }
    
    f_close(&file);
    
    printf("Loaded capture from %s with %d transitions\n", 
           timestamp_str, capture.transition_count);
    return capture.transition_count > 0;
}