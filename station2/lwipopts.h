#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H
#pragma once

// System Settings
#ifndef NO_SYS
#define NO_SYS                      1
#endif
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0
#endif

// Memory Management
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1
#else
#define MEM_LIBC_MALLOC             0
#endif
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    32000

// Memory Pools
#define MEMP_NUM_SYS_TIMEOUT        32    // Added this critical setting
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

// Core Protocol Support
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1

// TCP Settings
#define TCP_WND                     (16 * TCP_MSS)
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_TCP_KEEPALIVE          1

// Network Interface
#define LWIP_NETIF_STATUS_CALLBACK  1
#define LWIP_NETIF_LINK_CALLBACK    1
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_NETIF_TX_SINGLE_PBUF   1
#define LWIP_NETCONN                0

// DHCP Configuration
#define LWIP_DHCP                   1
#define DHCP_DOES_ARP_CHECK         0
#define LWIP_DHCP_DOES_ACD_CHECK    0

// Statistics (disabled for production)
#define MEM_STATS                   0
#define SYS_STATS                   0
#define MEMP_STATS                  0
#define LINK_STATS                  0

// Debugging
#ifndef NDEBUG
#define LWIP_DEBUG                  1
#define LWIP_STATS                  1
#define LWIP_STATS_DISPLAY          1
#endif

// HTTPD Settings
#define LWIP_HTTPD                  1
#define LWIP_HTTPD_SSI              1
#define LWIP_HTTPD_CGI              1
#define LWIP_HTTPD_SSI_INCLUDE_TAG  0
#define LWIP_HTTPD_SUPPORT_WEBSOCKET 1

// SNTP Configuration
#define LWIP_SNTP                   1
#define SNTP_SERVER_DNS             1
#define SNTP_UPDATE_DELAY           3600000  // Update every hour (in ms)
#define SNTP_RETRY_TIMEOUT          3000     // Retry every 3 seconds on failure
#define SNTP_MAX_SERVERS            2        // Support multiple NTP servers
#define SNTP_SET_SYSTEM_TIME_US(sec, us) sntp_set_system_time_us(sec, us)

// Checksum and Platform
#define LWIP_CHKSUM_ALGORITHM       3
#define LWIP_PLATFORM_BYTESWAP      0
#define LWIP_RAND()                 ((u32_t)rand())

#endif /* _LWIPOPTS_H */