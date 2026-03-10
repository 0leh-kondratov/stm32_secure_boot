/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/stm32h7xx_it.c 
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
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
#include "stm32h7xx_it.h"
#include "main.h"
#include "cmsis_os2.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern ETH_HandleTypeDef EthHandle;
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  *         In GDB: p/x g_hardfault_hfsr, g_hardfault_cfsr, g_hardfault_bfar, g_hardfault_pc
  *         then x/i g_hardfault_pc to see faulting instruction.
  */
static volatile uint32_t g_hardfault_hfsr;
static volatile uint32_t g_hardfault_cfsr;
static volatile uint32_t g_hardfault_bfar;
static volatile uint32_t g_hardfault_pc;

void HardFault_Handler(void)
{
  g_hardfault_hfsr = SCB->HFSR;
  g_hardfault_cfsr = SCB->CFSR;
  g_hardfault_bfar = SCB->BFAR;
  /* Stacked PC (faulting instruction) is at MSP+24 on exception entry */
  {
    uint32_t *msp = (uint32_t *)__get_MSP();
    g_hardfault_pc = msp[6];
  }
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  *         В GDB: p/x g_usage_fault_cfsr — причина (UFSR в битах 16–31).
  *         Адрес сбойной инструкции: x/xw $sp+24  затем  x/i <значение>
  */
static volatile uint32_t g_usage_fault_cfsr;

void UsageFault_Handler(void)
{
  g_usage_fault_cfsr = (*((volatile uint32_t *)0xE000ED28U)); /* SCB->CFSR */
  while (1)
  {
  }
}


/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/******************************************************************************/
/*                 STM32H7xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32h7xx.s).                                               */
/******************************************************************************/
/**
  * @brief  This function handles Ethernet interrupt request.
  *         HAL_ETH_IRQHandler processes RX/TX complete and error interrupts;
  *         LwIP is notified via HAL callbacks (e.g. HAL_ETH_RxCpltCallback)
  *         which signal the ethernetif layer (semaphore) so the LwIP task
  *         can process received packets.
  * @param  None
  * @retval None
  */
void ETH_IRQHandler(void)
{
  HAL_ETH_IRQHandler(&EthHandle);
}


/**
  * @}
  */ 

/**
  * @}
  */


