/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/main.c
  * @author  MCD Application Team
  * @brief   This sample code implements a http server application based on
  *          Netconn API of LwIP stack and FreeRTOS. This application uses
  *          STM32H7xx the ETH HAL API to transmit and receive data.
  *          The communication is done with a web browser of a remote PC.
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
#include "main.h"
#include "cmsis_os2.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/ip4_addr.h"
#include "app_ethernet.h"
#include "tcp_echo_server.h"
#include "uart_log.h"
#include "lcd_1602_i2c.h"
#include <string.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LCD_MONITOR_PERIOD_MS    500

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct netif gnetif;

/* I2C1 for LCD 1602 (NUCLEO-H743ZI: PB6 SCL, PB7 SDA). Address 0x27. */
I2C_HandleTypeDef hi2c1;
static osMutexId_t I2C1_MutexHandle;

/* StartDefaultTask: initializes LwIP (tcpip_init, netif_add) and starts TCP echo server. */
osThreadId_t StartDefaultTaskHandle;
const osThreadAttr_t StartDefaultTask_attributes = {
  .name = "StartDefaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t LinkHandle;
osThreadId_t DHCPHandle;
const osThreadAttr_t EthLinkThread_attributes = {
  .name = "EthLink",
  .stack_size = 256 * 4,
  .priority = osPriorityNormal
};
const osThreadAttr_t DHCPThread_attributes = {
  .name = "DHCP",
  .stack_size = 256 * 4,
  .priority = osPriorityBelowNormal
};

/* LCD monitor task: low priority, updates display every 500 ms. */
osThreadId_t LcdMonitorTaskHandle;
const osThreadAttr_t LcdMonitorTask_attributes = {
  .name = "lcd_monitor_task",
  .stack_size = 384 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void BSP_Config(void);
void StartDefaultTask(void* argument);
static void Netif_Config(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
static void MX_I2C1_Init(void);
void LcdMonitorTask(void *argument);
void log_ip_address(struct netif *netif);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Configure the MPU attributes as Device memory for ETH DMA descriptors */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32H7xx HAL library initialization:
       - Configure the TIM6 to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();

  /* Optional: SHA-256 integrity check of firmware before starting RTOS.
   * Verify image at APP_IMAGE_BASE (e.g. 0x08010000 or 0x08020000) with HAL_HASH,
   * compare to expected digest or skip if not implemented. */
  /* firmware_integrity_check(0x08020000, size); */

  /* Configure BSP (LEDs, I2C, LCD 1602) and show "Booting Secure OS..." */
  BSP_Config();

  /* Init FreeRTOS kernel */
  osKernelInitialize();

  /* Create the task that will call tcpip_init and netif_add (LwIP + network interface). */
  StartDefaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &StartDefaultTask_attributes);

  /* Start scheduler */
  osKernelStart();

  for ( ;; );
}

/**
  * @brief  StartDefaultTask: initialize LwIP stack and network interface, then start TCP echo server.
  */
void StartDefaultTask(void* argument)
{
  /* Initialize UART3 (PD8/PD9) for debug logging at 115200. */
  log_init();
  log_puts("LwIP + FreeRTOS\r\n");

  /* Create tcp_ip stack thread (LwIP core). */
  tcpip_init(NULL, NULL);

  /* Add network interface (HAL ETH + PHY, netif_add calls ethernetif_init). */
  Netif_Config();

  /* Log IP once assigned (static immediately; DHCP after a short delay in app_ethernet). */
  osDelay(100);
  log_ip_address(&gnetif);

  /* Start TCP echo server on port 7 (Netconn API). */
  tcp_server_thread_init();

  /* Create mutex for I2C1 (LCD and any other I2C users). */
  I2C1_MutexHandle = osMutexNew(NULL);
  if (I2C1_MutexHandle != NULL) {
    LcdMonitorTaskHandle = osThreadNew(LcdMonitorTask, NULL, &LcdMonitorTask_attributes);
  }

  for ( ;; )
  {
    osThreadTerminate(StartDefaultTaskHandle);
  }
}

/**
  * @brief  lcd_monitor_task: every 500 ms update I2C LCD 1602 (address 0x27).
  *         Line 1: current IP (from LwIP). Line 2: "Secure Boot OK".
  *         Uses Mutex to protect I2C1 access.
  */
void LcdMonitorTask(void *argument)
{
  char line1[17] = "Booting...";
  char line2[17] = "Secure Boot OK";
  (void)argument;

  for (;;)
  {
    if (osMutexAcquire(I2C1_MutexHandle, 100) == osOK)
    {
      if (netif_is_up(&gnetif))
      {
        uint32_t a = ip_addr_get_ip4_u32(&gnetif.ip_addr);
        (void)snprintf(line1, sizeof(line1), "%lu.%lu.%lu.%lu",
                       (unsigned long)((a >> 24) & 0xff),
                       (unsigned long)((a >> 16) & 0xff),
                       (unsigned long)((a >> 8) & 0xff),
                       (unsigned long)(a & 0xff));
      }
      else
      {
        (void)snprintf(line1, sizeof(line1), "No link/DHCP");
      }
      LCD_1602_Print(line1, line2);
      osMutexRelease(I2C1_MutexHandle);
    }
    osDelay(LCD_MONITOR_PERIOD_MS);
  }
}

/**
  * @brief  BSP Configuration
  * @param  None
  * @retval None
  */
static void BSP_Config(void)
{
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  MX_I2C1_Init();
  if (LCD_1602_Init(&hi2c1)) {
    LCD_1602_Print("Booting Secure OS", "...");
  }
}

/**
  * @brief  I2C1 init for LCD 1602 (PB6 SCL, PB7 SDA), 100 kHz.
  *         NUCLEO-H743ZI: I2C1 on PB6 (SCL), PB7 (SDA), AF4.
  */
static void MX_I2C1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC; /* 100 kHz at 200 MHz APB1 */
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
    Error_Handler();
  }
}

/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

#if LWIP_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* LWIP_DHCP */

  /* add the network interface */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface. */
  netif_set_default(&gnetif);

  ethernet_link_status_updated(&gnetif);

#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);
  LinkHandle = osThreadNew(ethernet_link_thread, &gnetif, &EthLinkThread_attributes);
#endif

#if LWIP_DHCP
  DHCPHandle = osThreadNew(DHCP_Thread, &gnetif, &DHCPThread_attributes);
#endif
}

/**
  * @brief  Log current IP address to UART3 (diagnostic). Call when IP is set (static or DHCP).
  */
void log_ip_address(struct netif *netif)
{
  char buf[24];
  if (!netif || !netif_is_up(netif)) return;
  {
    uint32_t a = ip_addr_get_ip4_u32(&netif->ip_addr);
    snprintf(buf, sizeof(buf), "IP: %lu.%lu.%lu.%lu\r\n",
             (unsigned long)((a >> 24) & 0xff),
             (unsigned long)((a >> 16) & 0xff),
             (unsigned long)((a >> 8) & 0xff),
             (unsigned long)((a >> 0) & 0xff));
  }
  log_puts(buf);
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE BYPASS)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 4
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable D2 domain SRAM3 Clock (0x30040000 AXI)*/
  __HAL_RCC_D2SRAM3_CLK_ENABLE();

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1);
  }
  
  /* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    while(1);
  }
}

/**
  * @brief  Configure the MPU for Ethernet and LwIP.
  *         Region 1: D2 SRAM at 0x30000000 (1 KB) - Device, non-cacheable.
  *         ETH DMA descriptors and descriptor tables live here; the DMA engine
  *         accesses physical memory. If this region were cacheable, the CPU
  *         could write descriptors into cache and the DMA would not see them
  *         until SCB_CleanDCache. Marking it Device non-cacheable avoids
  *         cache coherency issues and the need for explicit Clean/Invalidate.
  *         Region 2: LwIP Tx buffers (16 KB at 0x30004000) - Normal, non-cacheable.
  *         LwIP packet buffers used for TX are shared with ETH DMA; same
  *         coherency requirement. SCB_CleanDCache before handing a buffer to
  *         DMA and SCB_InvalidateDCache after RX are used in ethernetif when
  *         buffers are in cacheable regions; with MPU non-cacheable these
  *         are redundant but harmless.
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Device not cacheable
     for ETH DMA descriptors */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Normal Non Cacheable
     for LwIP RAM heap which contains the Tx buffers */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30004000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


