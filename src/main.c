#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"


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
    stdio_init_all();
    digital_comms_init();
    signal_generator_init();
    automation_init();

    while (true) {
        process_uart_command();
        sleep_ms(10); // Small delay to prevent tight looping
    }
}