#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hw_config.h"
#include "f_util.h"
#include "ff.h"
#include <stdbool.h>
#include <string.h>
#include "buddy5/ntp.h"

#define MAX_PROTOCOL_ENTRIES 100

typedef struct {
    uint32_t timestamp;
    uint8_t protocol_type;  // 1=PWM, 2=UART, 3=SPI, 4=I2C, 5=GPIO
    uint32_t data_value;    // Generic data value (e.g., frequency, baud rate)
    uint32_t secondary_value; // Additional data (e.g., duty cycle for PWM)
    bool success_flag;      // Indicates if the communication was successful
} ProtocolData;

// Function prototypes
FRESULT initialiseSD(void);
int readFile(const char* filename);
int writeDataToSD(const char* filename, const char* content, bool append);
int createNewFile(const char* filename);
bool save_protocol_data(uint8_t protocol_type, uint32_t data_value, 
                       uint32_t secondary_value, bool success_flag);
bool flush_data_buffer();
bool load_protocol_data(const char* filename, uint32_t start_time, uint32_t end_time, 
                       ProtocolData* output_buffer, int max_entries, int* entries_loaded);
#endif // SD_CARD_H