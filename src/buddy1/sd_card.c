#include <stdio.h>
#include "pico/stdlib.h"
#include "hw_config.h"
#include "f_util.h"
#include "ff.h"


// SD card initialization function stays the same
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

// Update function to use const char* for filename
int readFile(const char* filename) {
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

// Update function to use const char* for filename
int createNewFile(const char* filename) {
    FIL file;
    FRESULT fr;

    // Open file with create new/overwrite flags
    fr = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        printf("Failed to create new file: %d\n", (int)fr);
        return 0;
    }
    // Close the newly created file
    fr = f_close(&file);
    if (fr != FR_OK) {
        printf("Error closing new file: %d\n", (int)fr);
        return 0;
    }
    printf("New file created: %s\n", filename);
    return 1;
}
// Update function to use const char* for both filename and content
int writeDataToSD(const char* filename, const char* content, bool append) {
    FIL file;
    FRESULT fr;
    BYTE mode;

    // Set mode based on append flag
    if (append) {
        mode = FA_WRITE | FA_OPEN_APPEND;
    } else {
        mode = FA_WRITE | FA_OPEN_ALWAYS;
        // For non-append mode, need to handle existing file
        fr = f_open(&file, filename, mode);
        if (fr == FR_OK) {
            // If opening succeeded and we're not appending, truncate the file
            f_truncate(&file);
            // Reset the file pointer to the start
            f_lseek(&file, 0);
            f_close(&file);
        }
    }

    // Now open with the appropriate mode
    fr = f_open(&file, filename, mode);
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
    return 1;
}