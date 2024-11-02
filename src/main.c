#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy1/sd_card.h"
#include "buddy2/digital.h"
#include "buddy5/wifi.h"

// File name for storing pulse data
#define PULSE_FILE "pulses.txt"


int main() {
    stdio_init_all();
    sleep_ms(5000); // Give time for serial terminal to connect
    printf("\nPico Pirate Digital Pulse Capture and Replay\n");

    // Initialize NTP time sync
    if (!ntp_init()) {
        printf("Failed to sync time. Halting.\n");
        while (true) {
            tight_loop_contents();
        }
    }
    printf("Time synchronized successfully\n");

    // Initialize SD card
    if (FR_OK != initialiseSD()) {
        printf("Failed to initialize SD card. Halting.\n");
        while (true) {
            tight_loop_contents();
        }
    }
    printf("SD card initialized successfully\n");

    // Initialize digital pulse capture system
    digital_init();
    printf("Digital pulse capture system initialized\n");

    // Start capturing pulses
    printf("Starting pulse capture on GP2...\n");
    start_pulse_capture();

    // Wait for capture to complete
    printf("Waiting for 10 pulses...\n");
    while (!is_capture_complete()) {
        tight_loop_contents();
    }

    // Save captured pulses to SD card (will append)
    printf("Appending pulses to CSV file...\n");
    save_pulses_to_file(PULSE_FILE);

    // Load and replay only the latest pulse sequence
    printf("Loading latest pulse sequence...\n");
    if (load_pulses_from_file(PULSE_FILE)) {
        printf("Replaying latest pulse sequence twice on GP3...\n");
        replay_pulses(2);
    }
    
    printf("Sequence complete!\n");
    
    // Keep WiFi connection alive for more time syncs if needed
    while (true) {
        tight_loop_contents();
    }
    return 0;
}