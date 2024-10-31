#include <stdio.h>
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "hw_config.h"

int main() {
    stdio_init_all();
    sleep_ms(10000);
    puts("Hello, world!");

    // Initialize SD card driver first
    if (sd_init_driver() != 0) {
        printf("ERROR: SD card initialization failed\n");
        return -1;
    }

    // Get the SD card
    sd_card_t *sd = sd_get_by_num(0);
    if (!sd) {
        printf("ERROR: Could not get SD card\n");
        return -1;
    }

    // Mount the file system
    FATFS fs;
    FRESULT fr = f_mount(&fs, sd_get_drive_prefix(sd), 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return -1;
    }

    FIL fil;
    const char* const filename = "filename.txt";
    char full_path[32];
    snprintf(full_path, sizeof(full_path), "%s%s", sd_get_drive_prefix(sd), filename);
    
    fr = f_open(&fil, full_path, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        return -1;
    }

    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }

    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount(sd_get_drive_prefix(sd));
    puts("Goodbye, world!");
    
    while(1) {
        tight_loop_contents();
    }
}