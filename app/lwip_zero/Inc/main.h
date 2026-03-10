/**
  ******************************************************************************
  * @file    main.h
  * @brief   Header for LwIP Zero firmware.
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "stm32h7xx_nucleo.h"

/* LED1 on NUCLEO-H743ZI2: PB0, active-low (low = ON). */
#define LED1_PIN               GPIO_PIN_0
#define LED1_GPIO_PORT         GPIOB
#define LED1_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define LED1_ON_LEVEL          GPIO_PIN_RESET
#define LED1_OFF_LEVEL         GPIO_PIN_SET

/* Static fallback IP (used only when DHCP timeout happens). */
#define IP_ADDR0 ((uint8_t)192U)
#define IP_ADDR1 ((uint8_t)168U)
#define IP_ADDR2 ((uint8_t)0U)
#define IP_ADDR3 ((uint8_t)10U)

#define NETMASK_ADDR0 ((uint8_t)255U)
#define NETMASK_ADDR1 ((uint8_t)255U)
#define NETMASK_ADDR2 ((uint8_t)255U)
#define NETMASK_ADDR3 ((uint8_t)0U)

#define GW_ADDR0 ((uint8_t)192U)
#define GW_ADDR1 ((uint8_t)168U)
#define GW_ADDR2 ((uint8_t)0U)
#define GW_ADDR3 ((uint8_t)1U)

void Error_Handler(void);
uint8_t led1_is_on(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
