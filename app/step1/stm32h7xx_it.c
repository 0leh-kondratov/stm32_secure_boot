/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt handlers for step1 — FreeRTOS SysTick; UART not in IT mode.
  ******************************************************************************
  */

#include "main.h"
#include "FreeRTOS.h"

extern void xPortSysTickHandler(void);

/**
  * @brief  SysTick handler: drive FreeRTOS tick. HAL_IncTick() is in vApplicationTickHook().
  */
void SysTick_Handler(void)
{
  xPortSysTickHandler();
}

/**
  * @brief  USART3 interrupt — not used (UART in blocking mode). Kept for vector table.
  */
void USART3_IRQHandler(void)
{
  (void)0;
}
