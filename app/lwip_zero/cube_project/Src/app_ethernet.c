/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/app_ethernet.c
  * @author  MCD Application Team
  * @brief   Ethernet specific module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "main.h"
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "app_ethernet.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "lwip/ip4_addr.h"
#include "uart_log.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static volatile uint8_t s_led2_on = 0U;
static volatile uint8_t s_led3_on = 0U;
#if LWIP_DHCP
#define MAX_DHCP_TRIES  4
__IO uint8_t DHCP_state = DHCP_OFF;
#endif

/* Private function prototypes -----------------------------------------------*/
static void ethernet_log_netif_addr(const char *prefix, const struct netif *netif);
static void ethernet_set_led(uint32_t led, uint8_t on);
/* Private functions ---------------------------------------------------------*/

uint8_t ethernet_led2_is_on(void)
{
  return s_led2_on;
}

uint8_t ethernet_led3_is_on(void)
{
  return s_led3_on;
}

static void ethernet_set_led(uint32_t led, uint8_t on)
{
  if (on != 0U)
  {
    BSP_LED_On((Led_TypeDef)led);
  }
  else
  {
    BSP_LED_Off((Led_TypeDef)led);
  }

  if (led == (uint32_t)LED2)
  {
    s_led2_on = (on != 0U) ? 1U : 0U;
  }
  else if (led == (uint32_t)LED3)
  {
    s_led3_on = (on != 0U) ? 1U : 0U;
  }
}

static void ethernet_log_netif_addr(const char *prefix, const struct netif *netif)
{
  char ip_buf[16];
  char mask_buf[16];
  char gw_buf[16];
  char line[128];

  if (ip4addr_ntoa_r(netif_ip4_addr(netif), ip_buf, sizeof(ip_buf)) == NULL)
  {
    (void)snprintf(ip_buf, sizeof(ip_buf), "0.0.0.0");
  }
  if (ip4addr_ntoa_r(netif_ip4_netmask(netif), mask_buf, sizeof(mask_buf)) == NULL)
  {
    (void)snprintf(mask_buf, sizeof(mask_buf), "0.0.0.0");
  }
  if (ip4addr_ntoa_r(netif_ip4_gw(netif), gw_buf, sizeof(gw_buf)) == NULL)
  {
    (void)snprintf(gw_buf, sizeof(gw_buf), "0.0.0.0");
  }

  (void)snprintf(line, sizeof(line), "%s IP=%s MASK=%s GW=%s\r\n", prefix, ip_buf, mask_buf, gw_buf);
  log_puts(line);
}
/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
 {
    log_puts("[ETH] Link up\r\n");
#if LWIP_DHCP
    /* Update DHCP state machine */
    DHCP_state = DHCP_START;
#else
    ethernet_set_led((uint32_t)LED2, 1U);
    ethernet_set_led((uint32_t)LED3, 0U);
#endif /* LWIP_DHCP */
  }
  else
  {
    log_puts("[ETH] Link down\r\n");
#if LWIP_DHCP
    /* Update DHCP state machine */
    DHCP_state = DHCP_LINK_DOWN;
#else
    ethernet_set_led((uint32_t)LED2, 0U);
    ethernet_set_led((uint32_t)LED3, 1U);
#endif /* LWIP_DHCP */
  }
}

#if LWIP_DHCP
/**
  * @brief  DHCP Process
  * @param  argument: network interface
  * @retval None
  */
void DHCP_Thread(void* argument)
{
  struct netif *netif = (struct netif *) argument;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
  struct dhcp *dhcp;

  for (;;)
  {
    switch (DHCP_state)
    {
    case DHCP_START:
      {
        log_puts("[DHCP] Start\r\n");
        ip_addr_set_zero_ip4(&netif->ip_addr);
        ip_addr_set_zero_ip4(&netif->netmask);
        ip_addr_set_zero_ip4(&netif->gw);
        DHCP_state = DHCP_WAIT_ADDRESS;

        ethernet_set_led((uint32_t)LED2, 0U);
        ethernet_set_led((uint32_t)LED3, 0U);

        netifapi_dhcp_start(netif);
      }
      break;
    case DHCP_WAIT_ADDRESS:
      {
        if (dhcp_supplied_address(netif))
        {
          DHCP_state = DHCP_ADDRESS_ASSIGNED;
          ethernet_log_netif_addr("[DHCP] Assigned", netif);

          ethernet_set_led((uint32_t)LED2, 1U);
          ethernet_set_led((uint32_t)LED3, 0U);
        }
        else
        {
          dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

          /* DHCP timeout */
          if (dhcp->tries > MAX_DHCP_TRIES)
          {
            DHCP_state = DHCP_TIMEOUT;

            /* Static address used */
            IP_ADDR4(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
            IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
            IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
            netifapi_netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
            ethernet_log_netif_addr("[DHCP] Timeout, static", netif);

            ethernet_set_led((uint32_t)LED2, 1U);
            ethernet_set_led((uint32_t)LED3, 0U);
          }
        }
      }
      break;
  case DHCP_LINK_DOWN:
    {
      DHCP_state = DHCP_OFF;

      ethernet_set_led((uint32_t)LED2, 0U);
      ethernet_set_led((uint32_t)LED3, 1U);
    }
    break;
    default: break;
    }

    /* wait 500 ms */
    osDelay(500);
  }
}
#endif  /* LWIP_DHCP */