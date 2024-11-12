#include "swd.h"

void cycle() {
    gpio_put(SWCLK_PIN, 1);
    sleep_us(tfmhz);
    gpio_put(SWCLK_PIN, 0);
    sleep_us(tfmhz);
}

void write_swdio(uint32_t data, int num_bits) {
    for (int i = 0; i < num_bits; i++) {
        int bit = (data >> i) & 0x01;
        gpio_put(SWDIO_PIN, bit);
        cycle();
    }
}

int read_swdio() {
    int bit = gpio_get(SWDIO_PIN);
    gpio_put(SWCLK_PIN, 1);
    sleep_us(tfmhz);
    gpio_put(SWCLK_PIN, 0);
    sleep_us(tfmhz);
    return bit;
}

void swd_init() {
    gpio_init(SWCLK_PIN);
    gpio_set_dir(SWCLK_PIN, GPIO_OUT);
    gpio_init(SWDIO_PIN);
    gpio_set_dir(SWDIO_PIN, GPIO_OUT);
    gpio_put(SWCLK_PIN, 0);
    gpio_put(SWDIO_PIN, 0);

    write_swdio(0xFFFFFFFF, 32);
    write_swdio(0xFFFFF, 20);
    write_swdio(0xE79E, 16);
    write_swdio(0xFFFFFFFF, 32);
    write_swdio(0xFFFFF, 20);
    write_swdio(0x00, 20);

    write_swdio(0xFF, 8);
    write_swdio(0x6209F392, 32);
    write_swdio(0x86852D95, 32);
    write_swdio(0xE3DDAFE9, 32);
    write_swdio(0x19BC0EA2, 32);
    write_swdio(0x0, 4);
    write_swdio(0x1A, 8);
    write_swdio(0xFFFFFFFF, 32);
    write_swdio(0xFFFFF, 20);
    write_swdio(0x00, 8);
}

uint32_t read_idcode() {
    write_swdio(0xA5, 8);
    gpio_set_dir(SWDIO_PIN, GPIO_IN);
    cycle(); cycle(); cycle(); cycle();

    uint32_t idcode = 0;
    for (int i = 0; i < 32; i++) {
        int bit = read_swdio();
        idcode |= (bit << i);
    }
    gpio_set_dir(SWDIO_PIN, GPIO_OUT);
    cycle();
    write_swdio(0x00000000, 32);
    write_swdio(0x00000, 20);
    
    printf("SWD: Read IDCODE: 0x%08X\n", idcode);  // Add this line
    return idcode;
}