#ifndef NTP_H
#define NTP_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/apps/sntp.h"
#define NTP_SERVER "pool.ntp.org"

// Function declarations
void ntp_callback(void);
bool ntp_init(void);
uint32_t get_timestamp(void);
void format_timestamp(uint32_t timestamp, char* buffer, size_t size);

#endif