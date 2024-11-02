#include "digital.h"
#include "buddy1/sd_card.h"
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
    
    // Only process if this is the edge we're expecting
    if ((is_rising && capture.expecting_high) || (!is_rising && !capture.expecting_high)) {
        if (capture.transition_count < MAX_TRANSITIONS) {
            capture.transitions[capture.transition_count].time = current_time;
            capture.transitions[capture.transition_count].state = is_rising;
            capture.transition_count++;
            capture.expecting_high = !capture.expecting_high;  // Toggle expectation
            
            printf("Transition %d: %s at %lu us\n", 
                capture.transition_count,
                is_rising ? "HIGH" : "LOW",
                current_time - capture.start_time);
            
            // Check if we've captured all transitions (10 high + 10 low = 20)
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
    
    for (int replay = 0; replay < num_times; replay++) {
        uint32_t start_time = time_us_32();
        
        for (int i = 0; i < capture.transition_count; i++) {
            uint32_t relative_time = capture.transitions[i].time - capture.start_time;
            
            // Wait until it's time for this transition
            while (time_us_32() - start_time < relative_time) {
                tight_loop_contents();
            }
            gpio_put(DIGITAL_OUTPUT_PIN, capture.transitions[i].state);
        }
        
        // Add a small delay between replays
        sleep_ms(100);
    }
    
    // Ensure we end in a low state
    gpio_put(DIGITAL_OUTPUT_PIN, 0);
    printf("Replay complete\n");
}

void save_pulses_to_file(const char* filename) {
    if (capture.transition_count == 0) {
        printf("No transitions to save\n");
        return;
    }

    // Create new file
    createNewFile(filename);

    char buffer[256];
    // Save number of transitions as first line
    snprintf(buffer, sizeof(buffer), "TRANSITIONS:%d\n", capture.transition_count);
    writeDataToSD(filename, buffer, true);

    // Save each transition
    for (int i = 0; i < capture.transition_count; i++) {
        uint32_t relative_time = capture.transitions[i].time - capture.start_time;
        snprintf(buffer, sizeof(buffer), "%lu,%d\n", relative_time, capture.transitions[i].state);
        writeDataToSD(filename, buffer, true);
    }
    
    printf("Successfully saved %d transitions to %s\n", capture.transition_count, filename);
}

bool load_pulses_from_file(const char* filename) {
    FIL file;
    FRESULT fr;
    char line[100];
    
    // Reset current capture state
    memset(&capture, 0, sizeof(capture));
    capture.start_time = time_us_32();
    
    // Open file for reading
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        printf("Failed to open file for reading: %s\n", filename);
        return false;
    }
    
    // Read first line containing number of transitions
    if (f_gets(line, sizeof(line), &file)) {
        int num_transitions;
        if (sscanf(line, "TRANSITIONS:%d", &num_transitions) != 1) {
            printf("Invalid file format\n");
            f_close(&file);
            return false;
        }
    }

    // Read each transition
    while (f_gets(line, sizeof(line), &file) && capture.transition_count < MAX_TRANSITIONS) {
        uint32_t relative_time;
        int state;
        
        if (sscanf(line, "%lu,%d", &relative_time, &state) == 2) {
            capture.transitions[capture.transition_count].time = capture.start_time + relative_time;
            capture.transitions[capture.transition_count].state = (bool)state;
            capture.transition_count++;
        }
    }
    
    f_close(&file);
    printf("Loaded %d transitions from %s\n", capture.transition_count, filename);
    return capture.transition_count > 0;
}