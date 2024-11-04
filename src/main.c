#include <stdio.h>
#include "pico/stdlib.h"
#include "buddy1/sd_card.h"
#include "buddy2/digital.h"
#include "buddy5/wifi.h"

// File name for storing pulse data
#define PULSE_FILE "pulses.csv"

// Function prototypes
void print_menu(void);
void process_command(char cmd);

// Global flag to track if we've handled capture completion
bool capture_handled = true;

int main() {
    // Initialize stdio and USB
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
    printf("Time synchronized successfully! Wifi turning off\n");
    cyw43_arch_deinit();

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

    // Initial menu display
    print_menu();

    // Main loop - process single character commands immediately
    while (true) {
        int c = getchar_timeout_us(0); // Non-blocking character read
        if (c != PICO_ERROR_TIMEOUT) {
            process_command((char)c);
            if (c == '3') break; // Exit command
        }

        // If capture is complete and we haven't handled it yet
        if (!capture_handled && is_capture_complete()) {
            save_pulses_to_file(PULSE_FILE);
            printf("Capture complete and saved!\n");
            print_menu();
            capture_handled = true;
        }

        tight_loop_contents();
    }

    return 0;
}

void print_menu(void) {
    printf("\n=== Pulse Capture and Replay Menu ===\n");
    printf("Press a key to:\n");
    printf("1: Capture new pulses on GP%d\n", DIGITAL_INPUT_PIN);
    printf("2: Replay saved pulses on GP%d\n", DIGITAL_OUTPUT_PIN);
    printf("3: Exit\n");
    printf("Ready for command...\n");
}

void process_command(char cmd) {
    switch (cmd) {
        case '1':
            printf("\nStarting new pulse capture...\n");
            start_pulse_capture();
            capture_handled = false;  // Reset the flag when starting new capture
            break;

        case '2':
            printf("\nLoading and replaying most recent pulse sequence...\n");
            if (load_pulses_from_file(PULSE_FILE)) {
                replay_pulses(2); // Single replay for immediate response
                printf("Replay complete!\n");
                print_menu(); // Show menu only after replay is complete
            } else {
                printf("No pulse sequences found in storage.\n");
                printf("Please capture a sequence first using option 1.\n");
                print_menu(); // Show menu if replay failed
            }
            break;

        case '3':
            printf("Exiting program...\n");
            break;

        case '\n':
        case '\r':
            // Ignore newline/carriage return
            break;

        default:
            if (cmd >= ' ') { // Only show error for printable chars
                printf("Invalid command: '%c'\n", cmd);
                print_menu();
            }
            break;
    }
}