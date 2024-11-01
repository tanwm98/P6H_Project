// sd_card.h

#ifndef SD_CARD_H
#define SD_CARD_H

#include "ff.h"

// Function prototypes
FRESULT initialiseSD(void);
int readFile(char* filename);
int writeDataToSD(char* filename, char* content);

#endif // SD_CARD_H