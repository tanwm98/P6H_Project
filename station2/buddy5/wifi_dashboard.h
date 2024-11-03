#ifndef WIFI_DASHBOARD_H
#define WIFI_DASHBOARD_H

#include <stdint.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "dhcpserver/dhcpserver.h"
#include "dnsserver/dnsserver.h"
#include "hardware/pwm.h"


// WiFi Configuration
#define WIFI_SSID "Picussy"
#define WIFI_PASS "password123"
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1"
#define HTTP_PORT 42069

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
bool send_halt_command(void);
bool send_resume_command(void);
uint32_t get_device_idcode(void);
void handle_dashboard_events(void);

// Callback type for handling dashboard commands
typedef void (*dashboard_command_callback)(const char* command);
void register_dashboard_callback(dashboard_command_callback callback);

#endif // WIFI_DASHBOARD_H