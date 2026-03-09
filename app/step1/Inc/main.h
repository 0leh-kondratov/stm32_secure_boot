/**
  ******************************************************************************
  * @file    main.h
  * @brief   Step1 — LED1 + UART (ComIT style). NUCLEO-H743ZI: LED1=PB0, USART3 (PD8/PD9).
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

void Error_Handler(void);

extern UART_HandleTypeDef UartHandle;

/* LED1: NUCLEO-H743ZI PB0, active-low (low = ON) */
#define LED1_PIN                    GPIO_PIN_0
#define LED1_GPIO_PORT              GPIOB
#define LED1_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

/* USART3: COM1 on NUCLEO-H743ZI (PD8 TX, PD9 RX), 115200 8N1 */
#define USARTx                           USART3
#define USARTx_CLK_ENABLE()              __HAL_RCC_USART3_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOD_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOD_CLK_ENABLE()
#define USARTx_FORCE_RESET()             __HAL_RCC_USART3_FORCE_RESET()
#define USARTx_RELEASE_RESET()           __HAL_RCC_USART3_RELEASE_RESET()

#define USARTx_TX_PIN                    GPIO_PIN_8
#define USARTx_TX_GPIO_PORT              GPIOD
#define USARTx_TX_AF                     GPIO_AF7_USART3
#define USARTx_RX_PIN                    GPIO_PIN_9
#define USARTx_RX_GPIO_PORT              GPIOD
#define USARTx_RX_AF                     GPIO_AF7_USART3

#define USARTx_IRQn                      USART3_IRQn
#define USARTx_IRQHandler               USART3_IRQHandler

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
