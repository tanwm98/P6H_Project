#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <stdint.h>

void signal_generator_init();
void generate_signal(uint32_t frequency, uint32_t duration_ms);

#endif