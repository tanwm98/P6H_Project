#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Common settings used in most of the pico_w examples
// (see https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html)

// Allow DHCP to be enabled
#define LWIP_DHCP                  1

// Predefine platform endianness (this is for the pico)
#define NO_SYS                   1
#define LWIP_SOCKET              0
#define MEM_LIBC_MALLOC          0
#define MEM_ALIGNMENT            4
#define MEM_SIZE                 4000
#define MEMP_NUM_TCP_SEG         32
#define MEMP_NUM_ARP_QUEUE       10
#define PBUF_POOL_SIZE           24
#define LWIP_ARP                 1
#define LWIP_ETHERNET            1
#define LWIP_ICMP                1
#define LWIP_RAW                 0
#define TCP_WND                  (8 * TCP_MSS)
#define TCP_MSS                  1460
#define TCP_SND_BUF             (8 * TCP_MSS)
#define TCP_SND_QUEUELEN        ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK   1
#define LWIP_NETIF_HOSTNAME        1
#define LWIP_NETCONN             0
#define MEM_STATS                0
#define SYS_STATS               0
#define MEMP_STATS              0
#define LINK_STATS              0
#define LWIP_CHKSUM_ALGORITHM   3
#define LWIP_DHCP               1
#define LWIP_IPV4               1
#define LWIP_TCP                1
#define LWIP_UDP                1
#define LWIP_DNS                1
#define LWIP_TCP_KEEPALIVE      1
#define LWIP_NETIF_TX_SINGLE_PBUF  1
#define DHCP_DOES_ARP_CHECK     0
#define LWIP_DHCP_DOES_ACD_CHECK  0

// NTP specific settings
#define SNTP_SERVER_DNS          1
#define LWIP_SNTP               1
#define SNTP_UPDATE_DELAY       3600000  // Update every hour (in ms)
#define SNTP_RETRY_TIMEOUT      3000     // Retry every 3 seconds on failure
#define SNTP_MAX_SERVERS        2        // Support multiple NTP servers
#define SNTP_SET_SYSTEM_TIME_US(sec, us) 

// OS specific settings
#define LWIP_PLATFORM_BYTESWAP 0
#define LWIP_RAND() ((u32_t)rand())

#endif /* _LWIPOPTS_H */