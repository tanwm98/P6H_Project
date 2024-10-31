#include <string.h>
#include "hw_config.h"

/* Maker Pi Pico SD Card Hardware Configuration according to datasheet:
   GP10 -> SCK (Clock)
   GP11 -> SDI (MOSI - Master Out Slave In)
   GP12 -> SDO (MISO - Master In Slave Out)
   GP15 -> CSn (Chip Select)
*/

static spi_t spi = {
    .hw_inst = spi0,          // SPI0 component
    .miso_gpio = 12,          // SDO (MISO)
    .mosi_gpio = 11,          // SDI (MOSI)
    .sck_gpio = 10,           // SCK (Clock)
    .baud_rate = 1250000, // 12.5 MHz
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = 15,            // CSn (Chip Select)
};

static sd_card_t sd_cards[] = {
    {
        .type = SD_IF_SPI,
        .spi_if_p = &spi_if,
        .use_card_detect = false,     // The CD/DAT3 line is used for CSn in SPI mode
    }
};

/* This function is called by the library to get the number of SD cards */
size_t sd_get_num() {
    return count_of(sd_cards);
}

/* This function is called by the library to get a pointer to the SD card */
sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}