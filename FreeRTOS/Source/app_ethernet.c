/**
  ******************************************************************************
  * @file    app_ethernet.c
  * @brief   Ethernet specific module (official STM32 pattern).
  ******************************************************************************
  */

#include "lwip/opt.h"
#include "main.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "app_ethernet.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"

#if LWIP_DHCP
#define MAX_DHCP_TRIES 4
__IO uint8_t DHCP_state = DHCP_OFF;
#endif

void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
#if LWIP_DHCP
    DHCP_state = DHCP_START;
#else
    BSP_LED_On(LED2);
    BSP_LED_Off(LED3);
#endif
  }
  else
  {
#if LWIP_DHCP
    DHCP_state = DHCP_LINK_DOWN;
#else
    BSP_LED_Off(LED2);
    BSP_LED_On(LED3);
#endif
  }
}

#if LWIP_DHCP
void DHCP_Thread(void *argument)
{
  struct netif *netif = (struct netif *)argument;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;

  for (;;)
  {
    switch (DHCP_state)
    {
      case DHCP_START:
        ip_addr_set_zero_ip4(&netif->ip_addr);
        ip_addr_set_zero_ip4(&netif->netmask);
        ip_addr_set_zero_ip4(&netif->gw);
        DHCP_state = DHCP_WAIT_ADDRESS;

        BSP_LED_Off(LED2);
        BSP_LED_Off(LED3);

        netifapi_dhcp_start(netif);
        break;

      case DHCP_WAIT_ADDRESS:
        if (dhcp_supplied_address(netif))
        {
          DHCP_state = DHCP_ADDRESS_ASSIGNED;
          BSP_LED_On(LED2);
          BSP_LED_Off(LED3);
        }
        else
        {
          dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
          if (dhcp->tries > MAX_DHCP_TRIES)
          {
            DHCP_state = DHCP_TIMEOUT;
            IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
            IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
            IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
            netifapi_netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
            BSP_LED_On(LED2);
            BSP_LED_Off(LED3);
          }
        }
        break;

      case DHCP_LINK_DOWN:
        DHCP_state = DHCP_OFF;
        BSP_LED_Off(LED2);
        BSP_LED_On(LED3);
        break;

      default:
        break;
    }

    osDelay(500);
  }
}
#endif
