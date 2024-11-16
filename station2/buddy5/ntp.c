#include "ntp.h"

static absolute_time_t time_init;
static time_t base_time = 0;
static bool time_synced = false;

bool ntp_init(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER);
    sntp_init();
    
    // Wait for single time sync
    uint32_t start = time_us_32();
    while (base_time == 0 && (time_us_32() - start) < 10000000) {
        cyw43_arch_poll();
        sleep_ms(100);
    }

    if (base_time == 0) {
        printf("NTP sync failed\n");
        return false;
    }

    time_init = get_absolute_time();
    time_synced = true;
    sntp_stop();  // Stop SNTP service after initial sync
    printf("NTP synced and stopped\n");
    return true;
}

void sntp_set_system_time_us(uint64_t sec, uint32_t us) {
    base_time = (time_t)sec;
    time_init = get_absolute_time();
    
    char time_str[64];
    format_timestamp(get_timestamp(), time_str, sizeof(time_str));
    printf("Time set: %s\n", time_str);
}

uint32_t get_timestamp(void) {
    if (base_time == 0) {
        return 0;
    }
    absolute_time_t now = get_absolute_time();
    uint64_t elapsed_us = absolute_time_diff_us(time_init, now);
    uint32_t final_time = (uint32_t)(base_time + (elapsed_us / 1000000));
    return final_time;
}

void format_timestamp(uint32_t timestamp, char* buffer, size_t size) {
    time_t raw_time = (time_t)timestamp;
    struct tm* timeinfo = gmtime(&raw_time);
    
    // Adjust for UTC+8
    timeinfo->tm_hour += 8;
    mktime(timeinfo); // Normalize in case hours overflow
    
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S (UTC+8)", timeinfo);
}