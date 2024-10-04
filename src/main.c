#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "FreeRTOS.h"
#include "task.h"

void mainTask(__unused void *params) {
    while(1) {
        for(int i = 1; i <= 255; i++) {
            printf("Hello World: %d\n", i);
            sleep_ms(1000);
        }
    }

    /*

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }

    cyw43_arch_enable_sta_mode();

    */
}

void vLaunch() {
    TaskHandle_t task;
    xTaskCreate(mainTask, "MainTaskThread", 256, NULL, 1, &task);
    vTaskStartScheduler();
}

int main() {
    stdio_init_all();

    vLaunch();

    while(true);

    return 0;
}