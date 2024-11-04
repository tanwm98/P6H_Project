#include "wifi_dashboard.h"

#define MAX_BUFFER_SIZE 1024

// Static variables
static DashboardData current_data = {0};
static dhcp_server_t dhcp_server;
static dns_server_t dns_server;
static struct tcp_pcb *http_pcb = NULL;
static dashboard_command_callback command_handler = NULL;
static char response_buffer[MAX_BUFFER_SIZE];

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

    printf("Setting up access point '%s'...\n", WIFI_SSID);

    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);

    // Configure network
    struct netif *netif = netif_default;
    ip4_addr_t ip, netmask, gateway;
    
    printf("Setting up network configuration...\n");
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gateway, 192, 168, 4, 1);
    
    netif_set_addr(netif, &ip, &netmask, &gateway);

    // Print network configuration
    char ip_str[16], netmask_str[16], gateway_str[16];
    ip4addr_ntoa_r(&ip, ip_str, sizeof(ip_str));
    ip4addr_ntoa_r(&netmask, netmask_str, sizeof(netmask_str));
    ip4addr_ntoa_r(&gateway, gateway_str, sizeof(gateway_str));
    
    printf("\nNetwork Configuration:\n");
    printf("IP Address: %s\n", ip_str);
    printf("Netmask: %s\n", netmask_str);
    printf("Gateway: %s\n", gateway_str);
    printf("SSID: %s\n", WIFI_SSID);
    printf("Password: %s\n", WIFI_PASS);

    // Initialize DHCP server before DNS server
    printf("Initializing DHCP server...\n");
    dhcp_server_init(&dhcp_server, &ip, &netmask);
    printf("DHCP server initialized\n");

    // Initialize DNS server after DHCP
    printf("Initializing DNS server...\n");
    dns_server_init(&dns_server, &ip);
    printf("DNS server initialized\n");

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
    
    printf("\nWiFi Dashboard initialization complete!\n");
    printf("Connect to '%s' network with password '%s'\n", WIFI_SSID, WIFI_PASS);
    printf("Then navigate to http://%s:%d\n", ip_str, HTTP_PORT);
    
    return true;
}

void update_dashboard_data(DashboardData *data) {
    memcpy(&current_data, data, sizeof(DashboardData));
}

void register_dashboard_callback(dashboard_command_callback callback) {
    command_handler = callback;
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
            "<meta http-equiv=\"refresh\" content=\"1\">"
            "<style>"
                ".data-box { border: 1px solid #ccc; padding: 10px; margin: 10px; }"
                ".control-box { margin: 20px 0; }"
            "</style>"
        "</head>"
        "<body>"
            "<h1>Signal Analyzer Dashboard</h1>"
            
            "<div class=\"data-box\">"
                "<h2>Signal Analysis</h2>"
                "<p>PWM Frequency: %.2f Hz</p>"
                "<p>PWM Duty Cycle: %.1f%%</p>"
                "<p>Analog Frequency: %.2f Hz</p>"
                "<p>UART Baud Rate: %.0f bps</p>"
            "</div>"

            "<div class=\"data-box\">"
                "<h2>Debug Information</h2>"
                "<p>IDCODE: 0x%08X</p>"
                "<p>Device Status: %s</p>"
            "</div>"
        "</body>"
        "</html>",
        current_data.pwm_frequency,
        current_data.pwm_duty_cycle,
        current_data.analog_frequency,
        current_data.uart_baud_rate,
        current_data.idcode,
        current_data.device_halted ? "HALTED" : "RUNNING"
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
    
    // Handle different request types
    if (strstr(request, "GET /command?cmd=halt") != NULL) {
        if (command_handler) command_handler("halt");
    }
    else if (strstr(request, "GET /command?cmd=resume") != NULL) {
        if (command_handler) command_handler("resume");
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
    
    sleep_ms(1); // Short sleep to prevent tight loop
}