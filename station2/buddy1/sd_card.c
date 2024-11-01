#include <stdio.h>
#include "pico/stdlib.h"
#include "hw_config.h"
#include "f_util.h"
#include "ff.h"

// Function prototypes 
FRESULT initialiseSD(void);
int readFile(char* filename);
int writeDataToSD(char* filename, char* content);


// SD card initialization function 
FRESULT initialiseSD() {
    printf("Setting up SD card...\n");
    
    // Get SD card
    sd_card_t *pSD = sd_get_by_num(0);
    if (pSD == NULL) {
        printf("Failed to get SD card\n");
        return FR_DISK_ERR;
    }
    
    // Try to unmount first in case it was mounted
    f_unmount(pSD->pcName);
    
    printf("Mounting SD card...\n");
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %d\n", fr);
        return fr;
    }
    
    printf("SD card mounted successfully.\n");
    return fr;
}

// Function to read a file and display its content on the serial monitor 
int readFile(char* filename) {
    FIL file;
    FRESULT fr;
    char buf[100];

    // Open the file for reading
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        printf("f_open error: %d\n", fr);
        return false;
    }

    // Read the file line by line and display the content
    printf("Reading from file: %s\n", filename);
    while (f_gets(buf, sizeof(buf), &file)) {
        printf("%s", buf);
    }

    // Close the file after reading
    f_close(&file);
    printf("File closed after reading.\n");

    return 1;
}

// Write data to SD card 
int writeDataToSD(char* filename, char* content) {
    FIL file;
    FRESULT fr;

     // Open the file for writing (append mode) 
    fr = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK) {
        printf("f_open error: %d\n", (int)fr);
        return 0;
    }

    // Write the content to the file 
    if (f_printf(&file, "%s", content) < 0) {
        printf("f_printf failed\n");
        f_close(&file);
        return 0;
    }


    // Close the file after writing 
    fr = f_close(&file);
    if (fr != FR_OK) {
        printf("f_close error: %d\n", (int)fr);
        return 0;
    }

    printf("Data written to SD card successfully.\n");
    return 1;
}

