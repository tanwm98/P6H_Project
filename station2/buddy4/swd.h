#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifndef SWD_H
#define SWD_H

#define SWCLK_PIN 2
#define SWDIO_PIN 3
#define tfmhz 0.95

void cycle();
void write_swdio(uint32_t data, int num_bits);
int read_swdio();
void swd_init();
uint32_t read_idcode();

#endif