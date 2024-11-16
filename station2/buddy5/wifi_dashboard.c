#include "wifi_dashboard.h"
#include "ntp.h"
#include "buddy2/adc.h"
#include "buddy2/pwm.h"
#include "buddy3/protocol_analyzer.h"
#include "buddy4/swd.h"

#define MAX_BUFFER_SIZE 2048

// Static variables
static DashboardData current_data = {0};
static struct tcp_pcb *http_pcb = NULL;
static char response_buffer[MAX_BUFFER_SIZE];
static bool wifi_connected = false;

// Forward declarations for internal functions
static err_t http_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void update_http_response(char *response, const ip4_addr_t *client_ip);

bool init_wifi_dashboard(void) {
    printf("Starting WiFi initialization...\n");
    if (cyw43_arch_init()) {
        printf("CYW43 initialization failed!\n");
        return false;
    }

    printf("Connecting to WiFi network '%s'...\n", WIFI_SSID);
    
    // Enable station mode
    cyw43_arch_enable_sta_mode();
    
    // Connect to WiFi with retries
    int attempts = 0;
    while (attempts < MAX_RETRIES) {
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) == 0) {
            printf("WiFi connected!\n");
            wifi_connected = true;
            print_network_info();  // Add this here
            break;
        }
        printf("Connection attempt %d failed. Retrying...\n", attempts + 1);
        attempts++;
        sleep_ms(RETRY_DELAY_MS);
    }

    if (!wifi_connected) {
        printf("Failed to connect to WiFi after %d attempts\n", MAX_RETRIES);
        return false;
    }
    // Setup HTTP server
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
    ntp_init();
    return true;
}

void print_network_info(void) {
    ip4_addr_t ip = *netif_ip4_addr(netif_default);
    ip4_addr_t netmask = *netif_ip4_netmask(netif_default);
    ip4_addr_t gateway = *netif_ip4_gw(netif_default);

    char ip_str[16], netmask_str[16], gateway_str[16];
    ip4addr_ntoa_r(&ip, ip_str, sizeof(ip_str));
    ip4addr_ntoa_r(&netmask, netmask_str, sizeof(netmask_str));
    ip4addr_ntoa_r(&gateway, gateway_str, sizeof(gateway_str));
    
    printf("\nNetwork Configuration:\n");
    printf("IP Address: %s\n", ip_str);
    printf("Netmask: %s\n", netmask_str);
    printf("Gateway: %s\n", gateway_str);
    printf("\nNavigate to http://%s:%d to access the dashboard\n", ip_str, HTTP_PORT);
}

bool is_wifi_connected(void) {
    return wifi_connected;
}

void update_dashboard_data(DashboardData *data) {
    memcpy(&current_data, data, sizeof(DashboardData));
}

static void update_http_response(char *response, const ip4_addr_t *client_ip) {
    char client_ip_str[16];
    ip4addr_ntoa_r(client_ip, client_ip_str, sizeof(client_ip_str));

    snprintf(response, MAX_BUFFER_SIZE,
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
            "<meta charset=\"UTF-8\">"
            "<title>Signal Analyzer Dashboard</title>"
            "<style>"
                ".data-box{border:1px solid #ccc;padding:10px;margin:10px}"
                ".btn{padding:8px;margin:5px;cursor:pointer}"
                ".control-btn{background:#f0f0f0;border:1px solid #ccc}"
                ".active{background:#fcc}"
                ".idcode-btn{background:#4CAF50;color:white;border:none}"
                ".measurement{color:#666}"
            "</style>"
            "%s"  // Conditional auto-refresh meta tag
        "</head>"
        "<body>"
            "<h1>Signal Analyzer Dashboard</h1>"
            
            "<div class=\"data-box\">"
                "<h2>Controls</h2>"
                "<form method=\"POST\" action=\"/button\">"
                    "<button class=\"btn control-btn %s\" name=\"btn\" value=\"1\">PWM %s</button>"
                    "<div class=\"measurement\">Frequency: %.2f Hz | Duty Cycle: %.1f%%</div>"
                    
                    "<button class=\"btn control-btn %s\" name=\"btn\" value=\"2\">ADC %s</button>"
                    "<div class=\"measurement\">Frequency: %.2f Hz</div>"
                    
                    "<button class=\"btn control-btn %s\" name=\"btn\" value=\"3\">Protocol %s</button>"
                    "<div class=\"measurement\">Baud Rate: %.0f bps</div>"
                "</form>"
            "</div>"

            "<div class=\"data-box\">"
                "<h2>Debug Information</h2>"
                "<form method=\"POST\" action=\"/button\">"
                    "<button class=\"btn idcode-btn\" name=\"btn\" value=\"4\">Get IDCODE</button>"
                "</form>"
                "<div class=\"measurement\">IDCODE: 0x%08X</div>"
            "</div>"
        "</body>"
        "</html>",
        // Add auto-refresh meta tag only if any capture is active
        (current_data.pwm_active || current_data.adc_active || current_data.protocol_active) ? 
            "<meta http-equiv=\"refresh\" content=\"1\">" : "",
        current_data.pwm_active ? "active" : "",
        current_data.pwm_active ? "Stop" : "Start",
        current_data.pwm_frequency,
        current_data.pwm_duty_cycle,
        
        current_data.adc_active ? "active" : "",
        current_data.adc_active ? "Stop" : "Start",
        current_data.analog_frequency,
        
        current_data.protocol_active ? "active" : "",
        current_data.protocol_active ? "Stop" : "Start",
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
    
    // Simple POST handling
    if (strncmp(request, "POST /button", 11) == 0) {
        // Find the button value
        char *btn_pos = strstr(request, "btn=");
        if (btn_pos) {
            int button_val = btn_pos[4] - '0';
            handle_dashboard_button((DashboardButton)button_val);
        }
    }
    
    // Generate and send response
    update_http_response(response_buffer, &tpcb->remote_ip);
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

void handle_dashboard_events(void) {
    // Required for WiFi processing
    cyw43_arch_poll();
    
    // Check WiFi connection status
    if (wifi_connected && !cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA)) {
        printf("WiFi connection lost! Attempting to reconnect...\n");
        wifi_connected = false;
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, 
                                             CYW43_AUTH_WPA2_AES_PSK, 30000) == 0) {
            printf("WiFi reconnected!\n");
            wifi_connected = true;
            print_network_info();
        }
    }
    
    sleep_ms(1); // Short sleep to prevent tight loop
}

void handle_dashboard_button(DashboardButton button) {
    // Always update relevant data based on which button was pressed
    switch(button) {
        case BUTTON_PWM:
            if (!is_capturing()) {
                start_capture();
                current_data.pwm_active = true;
            } else {
                stop_capture();
                current_data.pwm_active = false;
            }
            // Update PWM metrics immediately after toggle
            if (current_data.pwm_active) {
                PWMMetrics pwm = get_pwm_metrics();
                current_data.pwm_frequency = pwm.frequency;
                current_data.pwm_duty_cycle = pwm.duty_cycle;
            } else {
                current_data.pwm_frequency = 0;
                current_data.pwm_duty_cycle = 0;
            }
            break;
            
        case BUTTON_ADC:
            if (!is_adc_capturing()) {
                adc_start_capture();
                current_data.adc_active = true;
                
            } else {
                adc_stop_capture();
                current_data.analog_frequency = 0;
                current_data.adc_active = false;
            }
            break;
        case BUTTON_PROTOCOL:
            if (!is_protocol_capturing()) {
                start_protocol_capture();
                current_data.protocol_active = true;
            } else {
                stop_protocol_capture();
                current_data.protocol_active = false;
            }
            // Update protocol metrics immediately after toggle
            if (current_data.protocol_active) {
                current_data.uart_baud_rate = get_uart_baud_rate();
            } else {
                current_data.uart_baud_rate = 0;
            }
            break;
            
        case BUTTON_IDCODE:
            // Get fresh IDCODE reading when button is pressed
            swd_init();  // Re-initialize SWD
            current_data.idcode = read_idcode();
            break;
    }
}