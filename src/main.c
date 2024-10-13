#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"


#include "buddy3/digital_comms.h"
#include "buddy3/signal_generator.h"
#include "buddy3/i2c/i2c.h"
#include "buddy3/automate.h"


// idk when we gonna use FreeRTOS tbh


// void mainTask(__unused void *params) {
//     while(1) {
//         for(int i = 1; i <= 255; i++) {
//             printf("Hello World!!: %d\n", i);
//             sleep_ms(1000);
//         }
//     }

    /*

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }

    cyw43_arch_enable_sta_mode();

    */
// }

// void vLaunch() {
//     TaskHandle_t task;
//     xTaskCreate(mainTask, "MainTaskThread", 256, NULL, 1, &task);
//     vTaskStartScheduler();
// }

int main() {
    stdio_init_all();  // Initialize standard I/O (for UART prints, etc.)
    printf("Starting main function\n");
    sleep_ms(2000);  // Give some time for serial to initialize
    // Perform an initial I2C scan at 100kHz baud rate to detect connected slaves
    
    i2c_simple_write_test();
    //i2c_scan(100000);

    // Initialize all the components of the system
    //digital_comms_init();      // Initialize UART and other digital communication
    //signal_generator_init();   // Initialize signal generator GPIOs
    //automation_init();         // Initialize automation processes (I2C setup, etc.)

    // Main loop for running automated sequences and handling UART commands
    while (true) {
        // Process UART commands (e.g., receiving instructions from the user)
        process_uart_command();

        // Optionally, you can trigger the automated sequence periodically or based on conditions.
        // This is an example of running the automated sequence after receiving UART data:
        run_automated_sequence();

        // Small delay to prevent tight looping
        sleep_ms(10);
    }
}