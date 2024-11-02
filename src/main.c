#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy2/digital.h"
#include "buddy1/sd_card.h"

// File name for storing pulse data
#define PULSE_FILE "pulses.txt"

int main() {
    // Initialize standard I/O
    stdio_init_all();
    sleep_ms(2000); // Give time for serial terminal to connect
    printf("\nPico Pirate Digital Pulse Capture and Replay\n");

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

    // Save captured pulses to SD card
    printf("Saving pulses to SD card...\n");
    save_pulses_to_file(PULSE_FILE);

    // Read back and verify the saved pulses
    printf("Reading pulses from file for verification...\n");
    readFile(PULSE_FILE);

    // Replay the pulses twice
    printf("Replaying pulses twice on GP3...\n");
    replay_pulses(2);
    printf("Sequence complete!\n");
    while (true) {
        tight_loop_contents();
    }
    return 0;
}