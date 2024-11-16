#ifndef WIFI_DASHBOARD_H
#define WIFI_DASHBOARD_H

#include <stdint.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"

// WiFi Configuration
#define HTTP_PORT 80
#define WIFI_SSID "TP-Link_1246"
#define WIFI_PASSWORD "1234512345"

// Maximum connection attempts
#define MAX_RETRIES 10
#define RETRY_DELAY_MS 1000

// Structure to hold all dashboard data
typedef struct {
    // Signal analysis data
    float pwm_frequency;
    float pwm_duty_cycle;
    float analog_frequency;
    float uart_baud_rate;
    float last_capture_time;
    uint32_t transition_count;
    uint32_t max_transitions;
    bool capture_active;
    bool replay_active;
    bool pwm_active;
    bool adc_active;
    bool protocol_active;
    bool error_state;
    const char* error_message;
    uint32_t idcode;
} DashboardData;

// Function declarations
bool init_wifi_dashboard(void);
void update_dashboard_data(DashboardData *data);
void handle_dashboard_events(void);
bool is_wifi_connected(void);
void print_network_info(void);


#endif // WIFI_DASHBOARD_H