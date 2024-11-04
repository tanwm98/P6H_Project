#include "wifi.h"

static time_t current_time = 0;
static absolute_time_t time_init;

void sntp_set_system_time_us(uint64_t sec, uint32_t us) {
    // Extract the seconds portion - no need to divide by 1000000 as it's already in seconds
    current_time = (time_t)sec;
    time_init = get_absolute_time();
        
    char time_str[64];
    format_timestamp(current_time, time_str, sizeof(time_str));
    printf("NTP time sync successful: %s\n", time_str);
}

bool ntp_init(void) {
    printf("Initializing WiFi...\n");
    if (cyw43_arch_init()) {
        printf("Failed to initialize cyw43_arch\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();
    
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, 
                                         CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to WiFi\n");
        return false;
    }
    struct netif *netif = netif_default;
    printf("Connected to WiFi\n");
    printf("IP Configuration:\n");
    printf("  IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
    printf("  Subnet Mask: %s\n", ip4addr_ntoa(netif_ip4_netmask(netif)));
    printf("  Gateway: %s\n", ip4addr_ntoa(netif_ip4_gw(netif)));
    // Initialize SNTP
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER);
    sntp_init();
    
    // Wait for time sync (timeout after 10 seconds)
    uint32_t start = time_us_32();
    while (current_time == 0 && (time_us_32() - start) < 10000000) {
        // Process any pending network events
        cyw43_arch_poll();
        sleep_ms(100);
    }

    if (current_time == 0) {
        printf("Failed to sync time with NTP server\n");
        return false;
    }

    return true;
}


uint32_t get_timestamp(void) {
    if (current_time == 0) {
        return 0;
    }
    absolute_time_t now = get_absolute_time();
    uint64_t elapsed_us = absolute_time_diff_us(time_init, now);
    uint32_t final_time = (uint32_t)(current_time + (elapsed_us / 1000000));
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