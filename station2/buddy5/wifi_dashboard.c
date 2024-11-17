#include "wifi_dashboard.h"

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

static err_t tcp_server_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    if (send_state.sent >= send_state.len) {
        // All data sent, close connection
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Calculate remaining data and next chunk size
    uint32_t remaining = send_state.len - send_state.sent;
    uint16_t chunk_size = MIN(1400, remaining); // 1400 to ensure we're under MTU with headers

    // Send next chunk
    const char *chunk_start = send_state.data + send_state.sent;
    err_t err = tcp_write(tpcb, chunk_start, chunk_size, 
                         remaining > chunk_size ? TCP_WRITE_FLAG_MORE : 0);
    
    if (err == ERR_OK) {
        send_state.sent += chunk_size;
        tcp_output(tpcb);
    } else {
        printf("Failed to write chunk: %d\n", err);
        tcp_close(tpcb);
    }

    return ERR_OK;
}
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
        // Configure PCB
    tcp_nagle_disable(http_pcb); // Disable Nagle's algorithm
    ip_set_option(http_pcb, SOF_KEEPALIVE);
    
    // Set TCP buffer sizes
    tcp_sndbuf(http_pcb);
    tcp_accept(http_pcb, http_server_accept);
    printf("HTTP server listening on port %d\n", HTTP_PORT);
    ntp_init();
    return true;
}

void print_network_info(void) {
    ip4_addr_t ip = *netif_ip4_addr(netif_default);
    char ip_str[16];
    ip4addr_ntoa_r(&ip, ip_str, sizeof(ip_str));
    printf("\nNavigate to http://%s:%d to access the dashboard\n", ip_str, HTTP_PORT);
}

bool is_wifi_connected(void) {
    return wifi_connected;
}

void update_dashboard_data(DashboardData *data) {
    memcpy(&current_data, data, sizeof(DashboardData));
}

static void update_http_response(char *response, const ip4_addr_t *client_ip) {
    // Only refresh for PWM, ADC, and protocol analysis - NOT for digital capture
    const char *refresh_meta = (current_data.pwm_active || current_data.adc_active || 
                              current_data.protocol_active) ? 
        "<meta http-equiv=\"refresh\" content=\"1\">" : "";

    int bytes = snprintf(response, MAX_BUFFER_SIZE,
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
                ".replay-btn{background:#007bff;color:white;border:none;margin-left:10px}"
            "</style>"
            "%s"  // Refresh meta tag
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

                    "<button class=\"btn control-btn %s\" name=\"btn\" value=\"5\">Digital Capture %s</button>"
                    "<button class=\"btn replay-btn\" name=\"btn\" value=\"6\" %s>Replay on GP3</button>"
                    "<div class=\"measurement\">%s</div>"
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
        refresh_meta,  // Only refreshes for non-digital captures
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
        
        current_data.digital_active ? "active" : "",
        current_data.digital_active ? "Stop" : "Start",
        current_data.digital_active ? "disabled" : "",  // Disable replay during capture
        current_data.digital_active ? "Press button again to stop capture" : 
            (current_data.digital_transition_count > 0 ? "Ready to replay" : "Ready to capture"),
        current_data.idcode
    );
    printf("HTTP Response: %d bytes\n", bytes);
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

    // Handle POST request
    char *request = (char *)p->payload;
    if (strncmp(request, "POST /button", 11) == 0) {
        char *btn_pos = strstr(request, "btn=");
        if (btn_pos) {
            int button_val = btn_pos[4] - '0';
            handle_dashboard_button((DashboardButton)button_val);
        }
    }

    // Generate complete response first
    update_http_response(response_buffer, &tpcb->remote_ip);
    
    // Setup header
    char header[128];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n", strlen(response_buffer));

    // Initialize send state
    send_state.data = response_buffer;
    send_state.len = strlen(response_buffer);
    send_state.sent = 0;

    // Register sent callback
    tcp_sent(tpcb, tcp_server_sent_callback);

    // Send header first
    err_t write_err = tcp_write(tpcb, header, strlen(header), TCP_WRITE_FLAG_MORE);
    if (write_err != ERR_OK) {
        printf("Header write failed: %d\n", write_err);
        pbuf_free(p);
        return write_err;
    }

    // Start sending the response body
    uint16_t chunk_size = MIN(1400, send_state.len); // Leave room for TCP/IP headers
    write_err = tcp_write(tpcb, send_state.data, chunk_size, TCP_WRITE_FLAG_MORE);
    if (write_err != ERR_OK) {
        printf("Initial body write failed: %d\n", write_err);
    } else {
        send_state.sent = chunk_size;
    }

    tcp_output(tpcb);
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

        case BUTTON_DIGITAL:
            if (!current_data.digital_active) {
                // Starting new capture
                digital_init();
                start_pulse_capture();
                current_data.digital_active = true;
                current_data.digital_transition_count = 0;
                current_data.last_transition_time = 0;
                current_data.last_state = false;
            } else {
                // Manual stop requested
                current_data.digital_active = false;
                // Get final state
                uint8_t count;
                const Transition* transitions = get_captured_transitions(&count);
                if (count > 0) {
                    current_data.digital_transition_count = count;
                    current_data.last_state = transitions[count-1].state;
                    current_data.last_transition_time = transitions[count-1].time;
                    save_pulses_to_file(PULSE_FILE); // Auto-save on manual stop
                }
                printf("Digital capture stopped manually. Captured %d transitions\n", 
                       current_data.digital_transition_count);
            }
            break;
            
        case BUTTON_REPLAY:
            if (!current_data.digital_active && current_data.digital_transition_count > 0) {
                printf("Starting replay on GP3...\n");
                replay_pulses(1);  // Replay once
            } else {
                printf("Cannot replay: %s\n", 
                    current_data.digital_active ? "Capture still active" : "No transitions captured");
            }
            break;
    }
}