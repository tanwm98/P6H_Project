#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "dhcpserver/dhcpserver.h"
#include "dnsserver/dnsserver.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

// Wi-Fi credentials for AP mode
#define WIFI_SSID "Picussy"
#define WIFI_PASS "password123"
#define AP_IP "192.168.4.1"
#define AP_NETMASK "255.255.255.0"
#define AP_GATEWAY "192.168.4.1"

// Port definitions
#define TCP_PORT 69
#define HTTP_PORT 42069
#define MAX_BUFFER_SIZE 1024

// Pins
#define SWCLK_PIN 2 // SWCLK pin
#define SWDIO_PIN 3 // SWDIO pin
#define tfmhz 0.95 // 1000us = 1ms, 1000ms = 1s
#define BUZZER_PIN 18 // Buzzer pin

// Function declarations
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t http_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);

void cycle() {
    gpio_put(SWCLK_PIN, 1); // Clock high
    sleep_us(tfmhz); // Timing for clock
    gpio_put(SWCLK_PIN, 0); // Clock low
    sleep_us(tfmhz);
}

void write_swdio(uint32_t data, int num_bits) {
    for (int i = 0; i < num_bits; i++) {
        int bit = (data >> i) & 0x01;
        gpio_put(SWDIO_PIN, bit); // Set SWDIO bit
        cycle();
    }
}

int read_swdio() {
    int bit = gpio_get(SWDIO_PIN);
    gpio_put(SWCLK_PIN, 1); // Clock high
    sleep_us(tfmhz); // Timing for clock
    gpio_put(SWCLK_PIN, 0); // Clock low
    sleep_us(tfmhz);
    return bit;
}

void swd_init() {
    gpio_init(SWCLK_PIN);
    gpio_set_dir(SWCLK_PIN, GPIO_OUT); // Set SWCLK as output
    gpio_init(SWDIO_PIN);
    gpio_set_dir(SWDIO_PIN, GPIO_OUT); // Start with SWDIO as output for initialization
    gpio_put(SWCLK_PIN, 0);
    gpio_put(SWDIO_PIN, 0);

    write_swdio(0xFFFFFFFF, 32);
    write_swdio(0xFFFFF, 20);
    write_swdio(0xE79E, 16);
    write_swdio(0xFFFFFFFF, 32);
    write_swdio(0xFFFFF, 20);
    write_swdio(0x00, 20);

    write_swdio(0xFF, 8);
    write_swdio(0x6209F392, 32);
    write_swdio(0x86852D95, 32);
    write_swdio(0xE3DDAFE9, 32);
    write_swdio(0x19BC0EA2, 32);
    write_swdio(0x0, 4);
    write_swdio(0x1A, 8);
    write_swdio(0xFFFFFFFF, 32);
    write_swdio(0xFFFFF, 20);
    write_swdio(0x00, 8);
}

uint32_t read_idcode() {
    write_swdio(0xA5, 8);
    gpio_set_dir(SWDIO_PIN, GPIO_IN);
    cycle(); cycle(); cycle(); cycle();

    uint32_t idcode = 0;
    for (int i = 0; i < 32; i++) {
        int bit = read_swdio();
        idcode |= (bit << i);
    }
    gpio_set_dir(SWDIO_PIN, GPIO_OUT);
    cycle();
    write_swdio(0x00000000, 32);
    write_swdio(0x00000, 20);
    return idcode;
}

void update_http_response(char *response, const ip4_addr_t *client_ip) {
    char client_ip_str[16];
    ip4addr_ntoa_r(client_ip, client_ip_str, sizeof(client_ip_str));

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    uint32_t idcode = "Idcodussy"; //RYAN ER CHANGE THIS TO read_idcode();
//                 ________  _____    _   ______________   
//                / ____/ / / /   |  / | / / ____/ ____/   
//               / /   / /_/ / /| | /  |/ / / __/ __/      
//              / /___/ __  / ___ |/ /|  / /_/ / /___      
//              \____/_/_/_/_/__|_/_/ |_/\____/_____/      
//                 / / / / ____/ __ \/ ____/               
//                / /_/ / __/ / /_/ / __/                  
//               / __  / /___/ _, _/ /___                  
//              /_/ /_/_____/_/_|_/_____/__   __________   
//                 / __ \ \/ /   |  / | / /  / ____/ __ \  
//                / /_/ /\  / /| | /  |/ /  / __/ / /_/ /  
//               / _, _/ / / ___ |/ /|  /  / /___/ _, _/   
//              /_/ |_| /_/_/  |_/_/ |_/  /_____/_/ |_|    
                                           

    snprintf(response, MAX_BUFFER_SIZE,
             "<!DOCTYPE html>"
             "<html lang=\"en\">"
             "<head><meta charset=\"UTF-8\"><title>Picussy Web Server</title></head>" 
             "<body>"
             "<h1>Hello, Picussy!</h1>" 
             "<p><strong>Pico IP:</strong> %s</p>"
             "<p><strong>Your IP:</strong> %s</p>"
             "<p><strong>IDCODE:</strong> 0x%08X</p>"
             "<p><strong>Current System Time:</strong> %s</p>"

             // Buzzer Button
             "<button id=\"buzzerButton\" onmousedown=\"startBuzzer()\" onmouseup=\"stopBuzzer()\">Buzzer</button>"

             // GPIO 21 Box
             "<div id=\"gpioBox\" style=\"width:100px; height:100px; background-color:green; margin-top:20px;\">GP21</div>"

             // JavaScript for Buzzer and GPIO 21 Status Update
             "<script>"
             "function startBuzzer() { fetch('/buzzer?state=on'); }"
             "function stopBuzzer() { fetch('/buzzer?state=off'); }"
             "function checkGPIO21() {"
             "   fetch('/gpio21')"
             "     .then(response => response.text())"
             "     .then(state => {"
             "         const box = document.getElementById('gpioBox');"
             "         box.style.backgroundColor = (state.trim() === 'pressed') ? 'red' : 'green';" // Change to red if pressed, green otherwise
             "     });"
             "} "
             "setInterval(checkGPIO21, 1000);"  // Check GPIO state every 1000ms
             "</script>"

             "</body>"
             "</html>",
             AP_IP, client_ip_str, idcode, time_str);
}



void print_ip_address(void) {
    printf("Assigned IP Address: %s\n", AP_IP);
}

// Buzzer PWM setup
void setup_pwm(uint pin)
{
    // Set the PWM frequency
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv(slice_num, 4.0f); // Set clock divider (optional)

}

void set_pwm_frequency(uint pin, float frequency)
{
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t freq = (uint32_t)(float)125000000 / (4 * frequency);
    pwm_set_wrap(slice_num, freq);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, freq / 2);
    pwm_set_enabled(slice_num, true);
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    printf("Setting up Wi-Fi AP...\n");

    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);

    struct netif *netif = netif_default;
    ip4_addr_t ip, netmask, gateway;
    ip4addr_aton(AP_IP, &ip);
    ip4addr_aton(AP_NETMASK, &netmask);
    ip4addr_aton(AP_GATEWAY, &gateway);
    netif_set_addr(netif, &ip, &netmask, &gateway);
    print_ip_address();

    // Initialize GPIO 21 as input
    gpio_init(21);
    gpio_set_dir(21, GPIO_IN);

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &ip, &netmask);
    printf("DHCP server initialized\n");

    dns_server_t dns_server;
    dns_server_init(&dns_server, &ip);
    printf("DNS server initialized\n");

    struct tcp_pcb *tcp_pcb = tcp_new();
    if (!tcp_pcb || tcp_bind(tcp_pcb, IP_ADDR_ANY, TCP_PORT) != ERR_OK) {
        printf("Error creating TCP server\n");
        return -1;
    }
    tcp_pcb = tcp_listen(tcp_pcb);
    tcp_accept(tcp_pcb, tcp_server_accept);
    printf("TCP server listening on port %d\n", TCP_PORT);

    struct tcp_pcb *http_pcb = tcp_new();
    if (!http_pcb || tcp_bind(http_pcb, IP_ADDR_ANY, HTTP_PORT) != ERR_OK) {
        printf("Error creating HTTP server\n");
        return -1;
    }
    http_pcb = tcp_listen(http_pcb);
    tcp_accept(http_pcb, http_server_accept);
    printf("HTTP server listening on port %d\n", HTTP_PORT);

    setup_pwm(BUZZER_PIN);

    while (true) {
        cyw43_arch_poll();
        sleep_ms(10000);
        print_ip_address();
    }

    cyw43_arch_deinit();
    return 0;
}


static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    printf("TCP connection accepted\n");
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    static char buffer[MAX_BUFFER_SIZE];
    static int buffer_len = 0;

    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    for (int i = 0; i < p->len; i++) {
        char ch = ((char *)p->payload)[i];
        if (ch == '\n' || ch == '\r') {
            buffer[buffer_len] = '\0';
            if (buffer_len > 0) {
                printf("Received message: %s\n", buffer);
                char response[256];
                snprintf(response, sizeof(response), "I hate %s\n", buffer);
                tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
                tcp_output(tpcb);
                printf("Sent reply: %s", response);
                buffer_len = 0;
            }
        } else if (buffer_len < MAX_BUFFER_SIZE - 1) {
            buffer[buffer_len++] = ch;
        }
    }

    pbuf_free(p);
    return ERR_OK;
}

static err_t http_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    printf("HTTP connection accepted\n");
    tcp_recv(newpcb, http_server_recv);
    return ERR_OK;
}

static err_t http_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    // After data is sent, close the connection
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t http_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;
    char response[MAX_BUFFER_SIZE];

    if (strstr(request, "GET /buzzer?state=on") != NULL) {
        // Button pressed - activate buzzer and confirm action
        printf("Button pressed - Buzzer activated\n");
        set_pwm_frequency(BUZZER_PIN, 1000.0f); // Set frequency before enabling PWM
        pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), true); // Start buzzer

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "Buzzer activated",
                 (int)strlen("Buzzer activated"));
        
    } else if (strstr(request, "GET /buzzer?state=off") != NULL) {
        // Button unpressed - deactivate buzzer and confirm action
        printf("Button unpressed - Buzzer deactivated\n");
        pwm_set_enabled(pwm_gpio_to_slice_num(BUZZER_PIN), false); // Stop buzzer

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "Buzzer deactivated",
                 (int)strlen("Buzzer deactivated"));
        
    } else if (strstr(request, "GET /gpio21") != NULL) {
        // GPIO 21 state check
        bool is_pressed = gpio_get(21);
        printf("GPIO 21 state: %s\n", is_pressed ? "unpressed" : "pressed");

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s", is_pressed ? 7 : 9,
                 is_pressed ? "pressed" : "unpressed");

    } else {
        // Regular browse - serve the HTML page
        printf("Regular browse - Serving HTML page\n");
        
        char body[MAX_BUFFER_SIZE];
        update_http_response(body, &tpcb->remote_ip);

        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %d\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s", (int)strlen(body), body);
    }

    // Send the response and set up callback for connection closure
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_sent(tpcb, http_server_sent); // Set callback to close after sending

    // Free the packet buffer
    pbuf_free(p);
    return ERR_OK;
}