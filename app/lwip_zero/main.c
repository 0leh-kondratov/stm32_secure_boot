/**
  ******************************************************************************
  * @file    main.c
  * @brief   lwip_zero: clean FreeRTOS + LwIP bring-up firmware.
  ******************************************************************************
  */

#include "main.h"
#include "cmsis_os2.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "netconn_page.h"
#include "uart_log.h"
#include <stdio.h>

struct netif gnetif;

osThreadId_t StartHandle;
osThreadId_t LinkHandle;
osThreadId_t DHCPHandle;

const osThreadAttr_t Start_attributes = {
  .name = "Start",
  .stack_size = 256 * 4,
  .priority = (osPriority_t)osPriorityNormal,
};

const osThreadAttr_t EthLinkThread_attributes = {
  .name = "EthLink",
  .stack_size = 256 * 4,
  .priority = osPriorityNormal,
};

const osThreadAttr_t DHCPThread_attributes = {
  .name = "DHCP",
  .stack_size = 256 * 4,
  .priority = osPriorityBelowNormal,
};

static void SystemClock_Config(void);
static void BSP_Config(void);
static void Netif_Config(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
static void StartThread(void *argument);
static void Log_Netif_Identity(void);
static void LED1_Init(void);

uint8_t led1_is_on(void)
{
  return (HAL_GPIO_ReadPin(LED1_GPIO_PORT, LED1_PIN) == LED1_ON_LEVEL) ? 1U : 0U;
}

int main(void)
{
  /* lwIP may access unaligned protocol fields on Cortex-M. */
  SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;

  MPU_Config();
  CPU_CACHE_Enable();

  HAL_Init();
  SystemClock_Config();
  BSP_Config();
  log_init();
  log_puts("[BOOT] lwip_zero start\r\n");

  osKernelInitialize();
  StartHandle = osThreadNew(StartThread, NULL, &Start_attributes);
  if (StartHandle == NULL)
  {
    Error_Handler();
  }

  osKernelStart();

  Error_Handler();
  for (;;)
  {
  }
}

static void StartThread(void *argument)
{
  (void)argument;

  tcpip_init(NULL, NULL);

  Netif_Config();
  Log_Netif_Identity();
  netconn_page_start(&gnetif);
  log_puts("[HTTP] Netconn server on port 80\r\n");

  for (;;)
  {
    osThreadTerminate(StartHandle);
  }
}

static void BSP_Config(void)
{
  LED1_Init();
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, LED1_OFF_LEVEL);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
}

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
  IP_ADDR4(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
  IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
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

static void Log_Netif_Identity(void)
{
  char line[96];
  (void)snprintf(line, sizeof(line), "[ETH] MAC=%02X:%02X:%02X:%02X:%02X:%02X\r\n",
                 (unsigned int)gnetif.hwaddr[0], (unsigned int)gnetif.hwaddr[1],
                 (unsigned int)gnetif.hwaddr[2], (unsigned int)gnetif.hwaddr[3],
                 (unsigned int)gnetif.hwaddr[4], (unsigned int)gnetif.hwaddr[5]);
  log_puts(line);
}

static void LED1_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  LED1_GPIO_CLK_ENABLE();
  gpio.Pin = LED1_PIN;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_PORT, &gpio);
}

static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
  {
  }

  __HAL_RCC_D2SRAM3_CLK_ENABLE();

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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  HAL_MPU_Disable();

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

  /* ETH descriptors region only (Cube reference uses 1KB device memory). */
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

  /* LwIP heap at 0x30004000. */
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

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void CPU_CACHE_Enable(void)
{
  SCB_EnableICache();
  SCB_EnableDCache();
}

void Error_Handler(void)
{
  while (1)
  {
  }
}

void _init(void)
{
}
