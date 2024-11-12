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
#define WIFI_SSID "Ming"
#define WIFI_PASSWORD "12341234"

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
    
    // Debug data
    uint32_t idcode;
    bool device_halted;
} DashboardData;

// Function declarations
bool init_wifi_dashboard(void);
void update_dashboard_data(DashboardData *data);
void handle_dashboard_events(void);
bool is_wifi_connected(void);
void print_network_info(void);

// Callback type for handling dashboard commands
typedef void (*dashboard_command_callback)(const char* command);
void register_dashboard_callback(dashboard_command_callback callback);
// Define SSI tags (max 8 chars per tag)
#define SSI_TAG_PWM_FREQ "pwmfreq"
#define SSI_TAG_PWM_DUTY "pwmduty"
#define SSI_TAG_ADC_FREQ "adcfreq"
#define SSI_TAG_UART_BPS "uartbps"
#define SSI_TAG_IDCODE   "idcode"

// WebSocket opcodes
#define WS_OP_CONT  0x0
#define WS_OP_TEXT  0x1
#define WS_OP_BIN   0x2
#define WS_OP_CLOSE 0x8
#define WS_OP_PING  0x9
#define WS_OP_PONG  0xA


// Function declarations
bool init_wifi_dashboard(void);
void update_dashboard_data(DashboardData *data);
void handle_dashboard_events(void);
#endif // WIFI_DASHBOARD_H