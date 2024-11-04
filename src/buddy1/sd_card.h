// sd_card.h

#ifndef SD_CARD_H
#define SD_CARD_H

#include "ff.h"
#include <stdbool.h>

// Function prototypes
FRESULT initialiseSD(void);
int readFile(char* filename);
int writeDataToSD(const char* filename, const char* content, bool append);
int createNewFile(const char* filename);

#endif // SD_CARD_H