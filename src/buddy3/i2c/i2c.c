#include "i2c.h"

#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pico/stdlib.h>

static void i2c_connect(uint32_t baud_rate)
{
    i2c_init(i2c0, baud_rate);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

static void i2c_disconnect()
{
    i2c_deinit(i2c0);
    gpio_set_function(SDA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(SCL_PIN, GPIO_FUNC_NULL);
}

static bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan(uint32_t baud_rate)
{
    i2c_connect(baud_rate);

    printf("I2C Bus Scan (Baud Rate: %u)\n", (unsigned int)baud_rate);
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c0, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }

    i2c_disconnect();
}

int i2c_write(uint8_t addr, const uint8_t *data, size_t len, bool nostop)
{
    i2c_connect(400000); // Standard 400kHz
    int result = i2c_write_blocking(i2c0, addr, data, len, nostop);
    
    printf("I2C Write: Addr 0x%02X, %d bytes\n", addr, (int)len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");

    i2c_disconnect();
    return result;
}

int i2c_read(uint8_t addr, uint8_t *data, size_t len, bool nostop)
{
    i2c_connect(400000); // Standard 400kHz
    int result = i2c_read_blocking(i2c0, addr, data, len, nostop);
    
    printf("I2C Read: Addr 0x%02X, %d bytes\n", addr, (int)len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");

    i2c_disconnect();
    return result;
}

void i2c_monitor(uint32_t duration_ms)
{
    i2c_connect(400000); // Standard 400kHz
    
    uint32_t start_time = time_us_32();
    while (time_us_32() - start_time < duration_ms * 1000) {
        bool sda = gpio_get(SDA_PIN);
        bool scl = gpio_get(SCL_PIN);
        
        printf("I2C Monitor: SDA=%d, SCL=%d\n", sda, scl);
        
        sleep_ms(10); // Adjust sampling rate as needed
    }
    
    i2c_disconnect();
}

void i2c_self_loop_test() // for testing
{
    printf("Starting I2C Self-Loop Test with address 0x%02X\n", I2C_SELF_ADDRESS);

    // Configure I2C for both master and slave operations
    i2c_init(i2c0, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Enable slave mode with our chosen address
    printf("Setting slave mode with address 0x%02X...\n", I2C_SELF_ADDRESS);
    i2c_set_slave_mode(i2c0, true, I2C_SELF_ADDRESS);
    

    // Test data
    uint8_t test_data[] = {0xA5, 0x5A, 0xF0, 0x0F};
    uint8_t received_data[sizeof(test_data)];

    // Write data (as master)
    printf("Sending data: ");
    for (size_t i = 0; i < sizeof(test_data); i++) {
        printf("%02X ", test_data[i]);
    }
    printf("\n");

    i2c_write_blocking(i2c0, I2C_SELF_ADDRESS, test_data, sizeof(test_data), false);

    // Read data (as slave)
    size_t bytes_read = 0;
    while (bytes_read < sizeof(test_data)) {
        uint8_t byte;
        int result = i2c_read_byte_raw(i2c0);
        if (result != PICO_ERROR_TIMEOUT) {
            byte = (uint8_t)result;
            received_data[bytes_read++] = byte;
        }
    }

    // Verify received data
    printf("Received data: ");
    bool test_passed = true;
    for (size_t i = 0; i < sizeof(test_data); i++) {
        printf("%02X ", received_data[i]);
        if (received_data[i] != test_data[i]) {
            test_passed = false;
        }
    }
    printf("\n");

    if (test_passed) {
        printf("Self-loop test passed!\n");
    } else {
        printf("Self-loop test failed.\n");
    }

    // Clean up
    i2c_set_slave_mode(i2c0, false, I2C_SELF_ADDRESS);
    i2c_deinit(i2c0);
    gpio_set_function(SDA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(SCL_PIN, GPIO_FUNC_NULL);
}

void i2c_simple_write_test() {
    printf("\nStarting Simple I2C Write Test\n");

    // Initialize I2C
    printf("Initializing I2C...\n");
    i2c_init(i2c0, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    printf("I2C peripheral status: 0x%08x\n", i2c0->hw->status);

    // Test data
    uint8_t test_data = 0xA5;

    // Write data to address 0x00 (general call address)
    printf("Attempting to write 0x%02X to address 0x00\n", test_data);

    // Clean up
    printf("Cleaning up...\n");
    i2c_deinit(i2c0);
    gpio_set_function(SDA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(SCL_PIN, GPIO_FUNC_NULL);

    printf("Simple I2C Write Test completed\n");
}