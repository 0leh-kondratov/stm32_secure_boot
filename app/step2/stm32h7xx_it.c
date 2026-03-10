/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt handlers for step2 — FreeRTOS SysTick; UART/I2C blocking.
  ******************************************************************************
  */

#include "main.h"
#include "FreeRTOS.h"

extern void xPortSysTickHandler(void);

void SysTick_Handler(void)
{
  xPortSysTickHandler();
}

void USART3_IRQHandler(void)
{
  (void)0;
}
