/**
  ******************************************************************************
  * @file    stm32h7xx_hal_msp.c
  * @brief   MSP init for step1 — USART3 (NUCLEO-H743ZI). Based on ST example.
  *          On H7, USART234578 clock source must be set (PCLK1) for correct BRR.
  ******************************************************************************
  */

#include "main.h"

void HAL_MspInit(void)
{
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (huart->Instance == USARTx)
  {
    /* USART234578 clock source: PCLK1 (required on STM32H7 for correct baud) */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART234578;
    PeriphClkInit.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    USARTx_TX_GPIO_CLK_ENABLE();
    USARTx_RX_GPIO_CLK_ENABLE();
    USARTx_CLK_ENABLE();

    GPIO_InitStruct.Pin       = USARTx_TX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed    = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = USARTx_TX_AF;
    HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = USARTx_RX_PIN;
    GPIO_InitStruct.Alternate = USARTx_RX_AF;
    HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);

    /* UART used in blocking mode only — no NVIC for USART IT */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USARTx)
  {
    USARTx_FORCE_RESET();
    USARTx_RELEASE_RESET();
    HAL_GPIO_DeInit(USARTx_TX_GPIO_PORT, USARTx_TX_PIN);
    HAL_GPIO_DeInit(USARTx_RX_GPIO_PORT, USARTx_RX_PIN);
  }
}
