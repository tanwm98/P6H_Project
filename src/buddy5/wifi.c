// #include "wifi.h"
// static bool ntp_synced = false;

// // Callback when NTP response is received
// static void ntp_callback(void *arg) {
//     printf("NTP time received\n");
//     ntp_synced = true;
// }

// bool wifi_connect(void) {
//     if (cyw43_arch_init()) {
//         printf("Failed to initialize WiFi\n");
//         return false;
//     }
    
//     printf("Connecting to WiFi...\n");
//     if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
//         printf("Failed to connect to WiFi\n");
//         return false;
//     }
    
//     printf("Connected to WiFi\n");
//     return true;
// }

// bool ntp_init(void) {
//     // Initialize RTC
//     datetime_t init_time = {
//         .year = 2024,
//         .month = 1,
//         .day = 1,
//         .dotw = 1,
//         .hour = 0,
//         .min = 0,
//         .sec = 0
//     };
//     rtc_init();
//     rtc_set_datetime(&init_time);

//     // Configure SNTP
//     sntp_init();
//     sntp_setoperatingmode(SNTP_OPMODE_POLL);
    
//     // Use IP address directly for NTP server (pool.ntp.org)
//     ip_addr_t ntp_server;
//     IP4_ADDR(&ntp_server, 162, 159, 200, 123); // pool.ntp.org IP address
//     sntp_setserver(0, &ntp_server);
    
//     // Wait for initial sync
//     int timeout = 30;  // 30 second timeout
//     while (timeout > 0) {
//         if (sntp_getreachability(0) != 0) {
//             printf("NTP sync successful\n");
//             return true;
//         }
//         sleep_ms(1000);
//         timeout--;
//     }
    
//     printf("NTP sync failed\n");
//     return false;
// }


// datetime_t get_current_time(void) {
//     datetime_t current_time;
//     rtc_get_datetime(&current_time);
//     return current_time;
// }

// uint64_t get_time_us(void) {
//     datetime_t t = get_current_time();
//     // Convert to Unix timestamp and add microseconds
//     struct tm time_tm = {
//         .tm_year = t.year - 1900,
//         .tm_mon = t.month - 1,
//         .tm_mday = t.day,
//         .tm_hour = t.hour,
//         .tm_min = t.min,
//         .tm_sec = t.sec
//     };
//     uint64_t timestamp = mktime(&time_tm);
//     return timestamp * 1000000ULL + time_us_32() % 1000000;
// }