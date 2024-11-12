#include "wifi.h"
#define MAX_BUFFER_SIZE 1024

// Static variables
static time_t current_time = 0;
static absolute_time_t time_init;
static DashboardData current_data = {0};
static struct tcp_pcb *http_pcb = NULL;
static char response_buffer[MAX_BUFFER_SIZE];
typedef void (*dashboard_command_callback)(const char* cmd);
static dashboard_command_callback command_handler = NULL;

// Forward declarations
static err_t http_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void update_http_response(char *response);

void sntp_set_system_time_us(uint64_t sec, uint32_t us) {
    current_time = (time_t)sec;
    time_init = get_absolute_time();
    
    char time_str[64];
    format_timestamp(current_time, time_str, sizeof(time_str));
    printf("NTP time sync successful: %s\n", time_str);
}

bool wifi_init(void) {
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

    // Print network configuration
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
    
    // Wait for time sync
    uint32_t start = time_us_32();
    while (current_time == 0 && (time_us_32() - start) < 10000000) {
        cyw43_arch_poll();
        sleep_ms(100);
    }

    if (current_time == 0) {
        printf("Failed to sync time with NTP server\n");
        return false;
    }

    // Initialize HTTP server
    printf("Setting up HTTP server...\n");
    http_pcb = tcp_new();
    if (!http_pcb) {
        printf("Failed to create HTTP PCB!\n");
        return false;
    }
    
    err_t err = tcp_bind(http_pcb, IP_ADDR_ANY, HTTP_PORT);
    if (err != ERR_OK) {
        printf("Failed to bind HTTP server to port %d! err=%d\n", HTTP_PORT, err);
        return false;
    }
    
    http_pcb = tcp_listen(http_pcb);
    if (!http_pcb) {
        printf("Failed to start HTTP listener!\n");
        return false;
    }
    
    tcp_accept(http_pcb, http_server_accept);
    printf("HTTP server listening on port %d\n", HTTP_PORT);
    printf("Dashboard available at http://%s\n", ip4addr_ntoa(netif_ip4_addr(netif)));

    return true;
}

void update_dashboard_data(DashboardData *data) {
    memcpy(&current_data, data, sizeof(DashboardData));
}

void register_dashboard_callback(dashboard_command_callback callback) {
    command_handler = callback;
}

static void update_http_response(char *response) {
    char timestamp_str[32] = "No capture";
    if (current_data.last_capture_time > 0) {
        format_timestamp(current_data.last_capture_time, timestamp_str, sizeof(timestamp_str));
    }

    char error_html[256] = "";
    if (current_data.error_state && current_data.error_message) {
        snprintf(error_html, sizeof(error_html),
            "<div class=\"error-message\">Error: %s</div>",
            current_data.error_message);
    }

    snprintf(response, MAX_BUFFER_SIZE,
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
            "<meta charset=\"UTF-8\">"
            "<title>Pico Pirate Dashboard</title>"
            "<meta http-equiv=\"refresh\" content=\"1\">"
            "<style>"
                "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }"
                ".data-box { background: white; border: 1px solid #ccc; padding: 15px; margin: 15px 0; "
                "border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
                "button { background-color: #4CAF50; color: white; border: none; padding: 10px 20px; "
                "margin: 5px; border-radius: 4px; cursor: pointer; }"
                "button:hover { background-color: #45a049; }"
                ".active { color: #4CAF50; font-weight: bold; }"
                ".inactive { color: #f44336; font-weight: bold; }"
                ".error-message { color: #f44336; padding: 10px; margin: 10px 0; border: 1px solid #f44336; "
                "border-radius: 4px; }"
                "h1 { color: #333; }"
                "h2 { color: #666; margin-top: 0; }"
            "</style>"
        "</head>"
        "<body>"
            "<h1>Pico Pirate Dashboard</h1>"
            "%s"  // Error message if any
            
            "<div class=\"data-box\">"
                "<h2>Controls</h2>"
                "<button onclick=\"location.href='/command?cmd=capture_pulses'\">Start Capture</button>"
                "<button onclick=\"location.href='/command?cmd=replay_pulses'\">Replay Pulses</button>"
            "</div>"

            "<div class=\"data-box\">"
                "<h2>Status</h2>"
                "<p>Capture: <span class=\"%s\">%s</span></p>"
                "<p>Replay: <span class=\"%s\">%s</span></p>"
                "<p>Last Capture: %s</p>"
                "<p>Transitions: %d / %d</p>"
            "</div>"

            "<div class=\"data-box\">"
                "<h2>Signal Analysis</h2>"
                "<p>PWM Status: <span class=\"%s\">%s</span></p>"
                "<p>PWM Frequency: %.2f Hz</p>"
                "<p>PWM Duty Cycle: %.1f%%</p>"
                "<p>ADC Status: <span class=\"%s\">%s</span></p>"
                "<p>Analog Frequency: %.2f Hz</p>"
                "<p>Protocol Status: <span class=\"%s\">%s</span></p>"
                "<p>UART Baud Rate: %.0f bps</p>"
            "</div>"

            "<div class=\"data-box\">"
                "<h2>Debug Information</h2>"
                "<p>IDCODE: 0x%08X</p>"
            "</div>"
        "</body>"
        "</html>",
        error_html,
        current_data.capture_active ? "active" : "inactive",
        current_data.capture_active ? "Active" : "Inactive",
        current_data.replay_active ? "active" : "inactive",
        current_data.replay_active ? "Active" : "Inactive",
        timestamp_str,
        current_data.transition_count,
        current_data.max_transitions,
        current_data.pwm_active ? "active" : "inactive",
        current_data.pwm_active ? "Active" : "Inactive",
        current_data.pwm_frequency,
        current_data.pwm_duty_cycle,
        current_data.adc_active ? "active" : "inactive",
        current_data.adc_active ? "Active" : "Inactive",
        current_data.analog_frequency,
        current_data.protocol_active ? "active" : "inactive",
        current_data.protocol_active ? "Active" : "Inactive",
        current_data.uart_baud_rate,
        current_data.idcode
    );
}

static err_t http_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_server_recv);
    return ERR_OK;
}

static err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;
    
    // Handle commands
    if (strstr(request, "GET /command?cmd=") != NULL) {
        if (command_handler) {
            if (strstr(request, "capture_pulses") != NULL) {
                command_handler("capture_pulses");
            }
            else if (strstr(request, "replay_pulses") != NULL) {
                command_handler("replay_pulses");
            }
        }
    }

    // Generate and send response
    update_http_response(response_buffer);
    
    // Send HTTP headers
    char header[128];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n", (int)strlen(response_buffer));
    
    tcp_write(tpcb, header, strlen(header), TCP_WRITE_FLAG_COPY);
    tcp_write(tpcb, response_buffer, strlen(response_buffer), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_sent(tpcb, http_server_sent);
    
    pbuf_free(p);
    return ERR_OK;
}

static err_t http_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

uint32_t get_timestamp(void) {
    if (current_time == 0) {
        return 0;
    }
    absolute_time_t now = get_absolute_time();
    uint64_t elapsed_us = absolute_time_diff_us(time_init, now);
    return (uint32_t)(current_time + (elapsed_us / 1000000));
}

void format_timestamp(uint32_t timestamp, char* buffer, size_t size) {
    time_t raw_time = (time_t)timestamp;
    struct tm* timeinfo = gmtime(&raw_time);
    
    // Adjust for UTC+8
    timeinfo->tm_hour += 8;
    mktime(timeinfo); // Normalize in case hours overflow
    
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S (UTC+8)", timeinfo);
}

void handle_wifi_events(void) {
    cyw43_arch_poll();
    sleep_ms(1);
}