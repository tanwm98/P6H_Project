#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Function declarations
void i2c_scan(uint32_t baud_rate);
int i2c_write(uint8_t addr, const uint8_t *data, size_t len, bool nostop);
int i2c_read(uint8_t addr, uint8_t *data, size_t len, bool nostop);
void i2c_monitor(uint32_t duration_ms);
void i2c_self_loop_test(void);
void i2c_simple_write_test(void);

// Constants
#define SDA_PIN 4
#define SCL_PIN 5
#define I2C_SELF_ADDRESS 0x30

#endif // I2C_H_