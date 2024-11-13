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
    
    // Debug data
    uint32_t idcode;
} DashboardData;

// Function declarations
bool init_wifi_dashboard(void);
void update_dashboard_data(DashboardData *data);
void handle_dashboard_events(void);
bool is_wifi_connected(void);
void print_network_info(void);

#define HTML_RESPONSE \
"<!DOCTYPE html>\n" \
"<html lang=\"en\">\n" \
"<head>\n" \
    "<meta charset=\"UTF-8\">\n" \
    "<title>Signal Analyzer Dashboard</title>\n" \
    "<style>\n" \
        ".data-box{border:1px solid #ccc;padding:10px;margin:10px}\n" \
        "#connection-status{padding:5px;margin:10px;border-radius:5px}\n" \
        ".connected{background-color:#90EE90}\n" \
        ".disconnected{background-color:#FFB6C1}\n" \
    "</style>\n" \
"</head>\n" \
"<body>\n" \
    "<h1>Signal Analyzer Dashboard</h1>\n" \
    "<div id=\"connection-status\" class=\"disconnected\">Disconnected</div>\n" \
    "<div class=\"data-box\">\n" \
        "<h2>Signal Analysis</h2>\n" \
        "<p>PWM Frequency: <span id=\"pwm-freq\">-</span> Hz</p>\n" \
        "<p>PWM Duty Cycle: <span id=\"pwm-duty\">-</span>%</p>\n" \
        "<p>Analog Frequency: <span id=\"analog-freq\">-</span> Hz</p>\n" \
        "<p>UART Baud Rate: <span id=\"uart-baud\">-</span> bps</p>\n" \
    "</div>\n" \
    "<div class=\"data-box\">\n" \
        "<h2>Debug Information</h2>\n" \
        "<p>IDCODE: <span id=\"idcode\">-</span></p>\n" \
    "</div>\n" \
    "<script>\n" \
    "let ws=null;\n" \
    "const statusDiv=document.getElementById('connection-status');\n" \
    "function connect(){\n" \
        "ws=new WebSocket(`ws://${window.location.host}`);\n" \
        "ws.onopen=()=>{\n" \
            "statusDiv.textContent='Connected';\n" \
            "statusDiv.className='connected';\n" \
        "};\n" \
        "ws.onclose=()=>{\n" \
            "statusDiv.textContent='Disconnected';\n" \
            "statusDiv.className='disconnected';\n" \
            "setTimeout(connect,1000);\n" \
        "};\n" \
        "ws.onmessage=(event)=>{\n" \
            "const data=JSON.parse(event.data);\n" \
            "document.getElementById('pwm-freq').textContent=data.pwm_freq.toFixed(2);\n" \
            "document.getElementById('pwm-duty').textContent=data.pwm_duty.toFixed(1);\n" \
            "document.getElementById('analog-freq').textContent=data.analog_freq.toFixed(2);\n" \
            "document.getElementById('uart-baud').textContent=Math.round(data.uart_baud);\n" \
            "document.getElementById('idcode').textContent='0x'+data.idcode.toString(16).padStart(8,'0');\n" \
        "};\n" \
    "}\n" \
    "connect();\n" \
    "</script>\n" \
"</body>\n" \
"</html>"


#endif // WIFI_DASHBOARD_H