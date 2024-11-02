#ifndef WIFI_H
#define WIFI_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/apps/sntp.h"

// NTP server settings
#define NTP_SERVER "pool.ntp.org"
#define WIFI_SSID "TP-Link_1246"
#define WIFI_PASSWORD "1234512345"
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970

// Function declarations
void ntp_callback(void);
bool ntp_init(void);
uint32_t get_timestamp(void);
void format_timestamp(uint32_t timestamp, char* buffer, size_t size);

#endif