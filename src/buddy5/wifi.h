#ifndef WIFI_H
#define WIFI_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "lwip/apps/sntp.h"
#include <time.h>
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/apps/httpd.h"

#define HTTP_PORT 80

// WiFi credentials
#define WIFI_SSID "Ming"
#define WIFI_PASSWORD "12341234"
#define NTP_SERVER "pool.ntp.org"

// Dashboard data structure
typedef struct {
    bool capture_active;
    bool replay_active;
    bool pwm_active;
    bool adc_active;
    bool protocol_active;
    bool error_state;
    const char* error_message;
    uint32_t last_capture_time;
    float pwm_frequency;
    float pwm_duty_cycle;
    float analog_frequency;
    float uart_baud_rate;
    uint32_t idcode;
    uint32_t transition_count;
    uint32_t max_transitions;
} DashboardData;

// Function type for dashboard callbacks
typedef void (*dashboard_command_callback)(const char* cmd);

// Function declarations
bool wifi_init(void);
uint32_t get_timestamp(void);
void format_timestamp(uint32_t timestamp, char* buffer, size_t size);
void update_dashboard_data(DashboardData *data);
void register_dashboard_callback(dashboard_command_callback callback);
void handle_wifi_events(void);


#endif // WIFI_H