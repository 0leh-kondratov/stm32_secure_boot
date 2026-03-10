/**
 * Official STM32H7 LwIP options baseline (Netconn + FreeRTOS).
 */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#define NO_SYS 0

/* Memory options */
#define MEM_ALIGNMENT 4
#define MEM_SIZE (14 * 1024)
#define LWIP_RAM_HEAP_POINTER (0x30004000)

/* Calculated from lwIP option set used by ST example */
#define MEMP_NUM_SYS_TIMEOUT 6
#define MEMP_NUM_TCP_PCB 10
#define MEMP_NUM_TCP_SEG TCP_SND_QUEUELEN

/* PBUF options */
#define PBUF_POOL_BUFSIZE 1536
#define LWIP_SUPPORT_CUSTOM_PBUF 1

/* IPv4 */
#define LWIP_IPV4 1

/* TCP */
#define LWIP_TCP 1
#define TCP_TTL 255
#define TCP_MSS (1500 - 40)
#define TCP_SND_BUF (4 * TCP_MSS)
#define TCP_WND (4 * TCP_MSS)

/* ICMP/DHCP/UDP */
#define LWIP_ICMP 1
#define LWIP_DHCP 1
#define LWIP_UDP 1
#define UDP_TTL 255

/* Statistics */
#define LWIP_STATS 0

/* Link callback */
#define LWIP_NETIF_LINK_CALLBACK 1

/* Checksum offload */
#define CHECKSUM_BY_HARDWARE
#ifdef CHECKSUM_BY_HARDWARE
#define CHECKSUM_GEN_IP 0
#define CHECKSUM_GEN_UDP 0
#define CHECKSUM_GEN_TCP 0
#define CHECKSUM_CHECK_IP 0
#define CHECKSUM_CHECK_UDP 0
#define CHECKSUM_CHECK_TCP 0
#define CHECKSUM_GEN_ICMP 1
#define CHECKSUM_CHECK_ICMP 0
#else
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1
#define CHECKSUM_GEN_ICMP 1
#define CHECKSUM_CHECK_ICMP 1
#endif

/* Sequential/socket APIs */
#define LWIP_NETCONN 1
#define LWIP_SOCKET 0
#define LWIP_NETIF_API 1

/* OS integration */
#define TCPIP_THREAD_NAME "TCP/IP"
#define TCPIP_THREAD_STACKSIZE 2048
#define TCPIP_MBOX_SIZE 6
#define DEFAULT_UDP_RECVMBOX_SIZE 6
#define DEFAULT_TCP_RECVMBOX_SIZE 6
#define DEFAULT_ACCEPTMBOX_SIZE 6
#define DEFAULT_THREAD_STACKSIZE 1024
#define TCPIP_THREAD_PRIO osPriorityHigh

#endif /* __LWIPOPTS_H__ */
