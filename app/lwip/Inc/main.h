/**
  ******************************************************************************
  * @file    main.h
  * @brief   app/lwip — FreeRTOS, LED1 blink, UART log, Ethernet, TCP/IP params to log.
  *          NUCLEO-H743ZI: LED1=PB0, USART3 (PD8/PD9), ETH on RMII.
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

void Error_Handler(void);

/* LED1: NUCLEO-H743ZI PB0, active-low (low = ON) */
#define LED1_PIN                    GPIO_PIN_0
#define LED1_GPIO_PORT              GPIOB
#define LED1_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

/* Static IP (used when LWIP_DHCP == 0) */
#define IP_ADDR0   ((uint8_t)192U)
#define IP_ADDR1   ((uint8_t)168U)
#define IP_ADDR2   ((uint8_t)0U)
#define IP_ADDR3   ((uint8_t)10U)

#define NETMASK_ADDR0   ((uint8_t)255U)
#define NETMASK_ADDR1   ((uint8_t)255U)
#define NETMASK_ADDR2   ((uint8_t)255U)
#define NETMASK_ADDR3   ((uint8_t)0U)

#define GW_ADDR0   ((uint8_t)192U)
#define GW_ADDR1   ((uint8_t)168U)
#define GW_ADDR2   ((uint8_t)0U)
#define GW_ADDR3   ((uint8_t)1U)

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
