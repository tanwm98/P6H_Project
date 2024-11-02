#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Essential memory configurations
#define MEM_ALIGNMENT           4
#define MEM_SIZE               4000
#define MEMP_NUM_TCP_SEG       32
#define MEMP_NUM_ARP_QUEUE     10
#define PBUF_POOL_SIZE         24

// TCP configurations
#define TCP_MSS                1460
#define TCP_WND                (8 * TCP_MSS)
#define TCP_SND_BUF           (8 * TCP_MSS)
#define TCP_SND_QUEUELEN       ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))

// SNTP specific configurations
#define SNTP_SERVER_DNS        1
#define LWIP_DHCP             1
#define LWIP_DNS              1
#define SNTP_UPDATE_DELAY     3600000  // Update every hour (in ms)
#define SNTP_RETRY_TIMEOUT    3000
#define SNTP_MAX_SERVERS      2
#define SNTP_SUPPORT          1

// General LWIP configurations
#define LWIP_IPV6             0
#define LWIP_NETCONN          0  // Not using netconn API
#define LWIP_UDP              1  // Required for SNTP
#define LWIP_TCP              1
#define LWIP_ICMP             1
#define LWIP_SNTP             1

// Debugging options (optional)
#define LWIP_DEBUG            1
#define SNTP_DEBUG            LWIP_DBG_ON

#endif /* _LWIPOPTS_H */