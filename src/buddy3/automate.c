#include "automate.h"
#include "signal_generator.h"
#include "hardware/uart.h"

#define UART_ID uart1

void automation_init() {
    // Scan for connected I2C slaves at 100kHz baud rate
    i2c_scan(100000); 
}

void run_automated_sequence() {
    // Send probing signals via the signal generator
    generate_signal(500, 500);

    // Communicate with an I2C slave (probe the system for data)
    uint8_t slave_address = 0x17;  // Set the slave address here
    
    // Example data write using the custom i2c_write function
    uint8_t data_to_send = 0x5A; // Some data to send
    int result = i2c_write(slave_address, &data_to_send, 1, false);

    if (result >= 0) {
        uart_puts(UART_ID, "I2C communication successful\n");
    } else {
        uart_puts(UART_ID, "I2C communication failed\n");
    }

    // Optionally, read data from the I2C slave using the custom i2c_read function
    uint8_t data_received[10];
    int read_result = i2c_read(slave_address, data_received, sizeof(data_received), false);
    if (read_result >= 0) {
        // Process the received data
        uart_puts(UART_ID, "I2C data received\n");
    }
}