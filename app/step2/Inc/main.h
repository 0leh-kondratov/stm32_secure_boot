/**
  ******************************************************************************
  * @file    main.h
  * @brief   Step2 — LED1 + UART + I2C scanner. NUCLEO-H743ZI: LED1=PB0, USART3 (PD8/PD9), I2C1 (PB6/PB7).
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
extern I2C_HandleTypeDef hi2c1;

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

/* I2C1: NUCLEO-H743ZI. Dwie opcje pinów (oba AF4, GPIOB):
 *   0 — PB6 SCL, PB7 SDA (Morpho)
* 1 - PB8 SCL, PB9 SDA (złącze CN7: D15=SCL, D14=SDA - jak w NUCLEO-F207ZG; I2C nie jest podłączone do CN8) */
#define I2Cx_USE_PB8_PB9               0

#define I2Cx                            I2C1
#define I2Cx_CLK_ENABLE()               __HAL_RCC_I2C1_CLK_ENABLE()
#define I2Cx_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2Cx_GPIO_PORT                  GPIOB
#define I2Cx_AF                         GPIO_AF4_I2C1

#if I2Cx_USE_PB8_PB9
#define I2Cx_SCL_PIN                    GPIO_PIN_8
#define I2Cx_SDA_PIN                    GPIO_PIN_9
#else
#define I2Cx_SCL_PIN                    GPIO_PIN_6
#define I2Cx_SDA_PIN                    GPIO_PIN_7
#endif

/* 1 = szelki wewnętrzne SDA/SCL; 0 = bez szelek (jeśli moduł już je posiada). */
#define I2Cx_GPIO_PULLUP                1

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
