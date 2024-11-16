#include "sd_card.h"

static ProtocolData data_buffer[MAX_PROTOCOL_ENTRIES];
static int buffer_count = 0;

bool save_protocol_data(uint8_t protocol_type, uint32_t data_value, 
                       uint32_t secondary_value, bool success_flag) {
    if (buffer_count >= MAX_PROTOCOL_ENTRIES) {
        printf("Data buffer full, saving to SD card...\n");
        flush_data_buffer();
        buffer_count = 0;
    }
    
    // Store in buffer using NTP timestamp
    data_buffer[buffer_count].timestamp = get_timestamp();
    data_buffer[buffer_count].protocol_type = protocol_type;
    data_buffer[buffer_count].data_value = data_value;
    data_buffer[buffer_count].secondary_value = secondary_value;
    data_buffer[buffer_count].success_flag = success_flag;
    buffer_count++;
    
    return true;
}

// Flush buffer to SD card
bool flush_data_buffer() {
    FIL file;
    FRESULT fr;
    char filename[32];
    char timestamp_str[64];
    uint32_t current_time = get_timestamp();
    
    // Generate filename based on current date from NTP timestamp
    format_timestamp(current_time, timestamp_str, sizeof(timestamp_str));
    // Extract date portion for filename (first 10 characters: YYYY-MM-DD)
    char date_str[11];
    strncpy(date_str, timestamp_str, 10);
    date_str[10] = '\0';
    // Remove dashes for filename
    char clean_date[9];
    int j = 0;
    for (int i = 0; i < 10; i++) {
        if (date_str[i] != '-') {
            clean_date[j++] = date_str[i];
        }
    }
    clean_date[j] = '\0';
    
    snprintf(filename, sizeof(filename), "proto_%s.csv", clean_date);
    
    // Check if file exists to determine if we need headers
    bool need_header = (f_open(&file, filename, FA_READ) != FR_OK);
    f_close(&file);
    
    // Open file for appending
    fr = f_open(&file, filename, FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("Failed to open file for writing: %s\n", filename);
        return false;
    }
    
    // Write header if new file
    if (need_header) {
        f_printf(&file, "timestamp,timestamp_readable,protocol,data_value,secondary_value,success\n");
    }
    
    // Write buffered data
    for (int i = 0; i < buffer_count; i++) {
        char entry_timestamp[64];
        format_timestamp(data_buffer[i].timestamp, entry_timestamp, sizeof(entry_timestamp));
        
        const char* protocol_name;
        switch (data_buffer[i].protocol_type) {
            case 1: protocol_name = "PWM"; break;
            case 2: protocol_name = "UART"; break;
            case 3: protocol_name = "SPI"; break;
            case 4: protocol_name = "I2C"; break;
            case 5: protocol_name = "GPIO"; break;
            default: protocol_name = "UNKNOWN"; break;
        }
        
        f_printf(&file, "%lu,%s,%s,%lu,%lu,%d\n",
                data_buffer[i].timestamp,
                entry_timestamp,
                protocol_name,
                data_buffer[i].data_value,
                data_buffer[i].secondary_value,
                data_buffer[i].success_flag);
    }
    
    f_close(&file);
    printf("Successfully saved %d records to %s\n", buffer_count, filename);
    return true;
}

// Load protocol data from file within time range
bool load_protocol_data(const char* filename, uint32_t start_time, uint32_t end_time, 
                       ProtocolData* output_buffer, int max_entries, int* entries_loaded) {
    FIL file;
    FRESULT fr;
    char line[256];
    *entries_loaded = 0;
    
    fr = f_open(&file, filename, FA_READ);
    if (fr != FR_OK) {
        printf("Failed to open file for reading: %s\n", filename);
        return false;
    }
    
    // Skip header
    f_gets(line, sizeof(line), &file);
    
    // Read and parse data
    uint32_t timestamp;
    char timestamp_str[64], protocol_str[10];
    uint32_t data_value, secondary_value;
    int success;
    
    while (f_gets(line, sizeof(line), &file) && *entries_loaded < max_entries) {
        if (sscanf(line, "%lu,%[^,],%[^,],%lu,%lu,%d",
                   &timestamp,
                   timestamp_str,
                   protocol_str,
                   &data_value,
                   &secondary_value,
                   &success) == 6) {
            
            if (timestamp >= start_time && timestamp <= end_time) {
                output_buffer[*entries_loaded].timestamp = timestamp;
                
                // Convert protocol string to type
                if (strcmp(protocol_str, "PWM") == 0) output_buffer[*entries_loaded].protocol_type = 1;
                else if (strcmp(protocol_str, "UART") == 0) output_buffer[*entries_loaded].protocol_type = 2;
                else if (strcmp(protocol_str, "SPI") == 0) output_buffer[*entries_loaded].protocol_type = 3;
                else if (strcmp(protocol_str, "I2C") == 0) output_buffer[*entries_loaded].protocol_type = 4;
                else if (strcmp(protocol_str, "GPIO") == 0) output_buffer[*entries_loaded].protocol_type = 5;
                
                output_buffer[*entries_loaded].data_value = data_value;
                output_buffer[*entries_loaded].secondary_value = secondary_value;
                output_buffer[*entries_loaded].success_flag = success;
                (*entries_loaded)++;
            }
        }
    }
    
    f_close(&file);
    printf("Loaded %d records from %s\n", *entries_loaded, filename);
    return true;
}

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