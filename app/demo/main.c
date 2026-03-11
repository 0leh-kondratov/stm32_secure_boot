/**
  ******************************************************************************
  * @file    main.c
  * @brief   Demo profile based on lwip_zero:
  *          FreeRTOS + LwIP + Netconn HTTP + I2C + USER button.
  ******************************************************************************
  */

#include "main.h"
#include "cmsis_os2.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "netconn_page.h"
#include "time_service.h"
#include "uart_log.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* I2C1 pin profile (same options as step2). */
#define DEMO_I2C_USE_PB8_PB9 0
#define DEMO_I2C             I2C1
#define DEMO_I2C_CLK_ENABLE()      __HAL_RCC_I2C1_CLK_ENABLE()
#define DEMO_I2C_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define DEMO_I2C_GPIO_PORT         GPIOB
#define DEMO_I2C_AF                GPIO_AF4_I2C1
#if DEMO_I2C_USE_PB8_PB9
#define DEMO_I2C_SCL_PIN GPIO_PIN_8
#define DEMO_I2C_SDA_PIN GPIO_PIN_9
#else
#define DEMO_I2C_SCL_PIN GPIO_PIN_6
#define DEMO_I2C_SDA_PIN GPIO_PIN_7
#endif

struct netif gnetif;

osThreadId_t StartHandle;
osThreadId_t LinkHandle;
osThreadId_t DHCPHandle;
osThreadId_t ControlHandle;

static I2C_HandleTypeDef h_i2c1;
static uint32_t s_button_press_count = 0U;

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

const osThreadAttr_t ControlThread_attributes = {
  .name = "Control",
  .stack_size = 256 * 4,
  .priority = osPriorityBelowNormal,
};

static void SystemClock_Config(void);
static void BSP_Config(void);
static void Netif_Config(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
static void StartThread(void *argument);
static void ControlThread(void *argument);
static void Log_Netif_Identity(void);
static void LED1_Init(void);
static void Demo_I2C1_Init(void);
static void Demo_I2C1_Scan(void);
static uint8_t Demo_Button_IsPressed(void);
static void Demo_Handle_ButtonPress(void);

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
  time_service_init();
  log_puts("[BOOT] demo start (lwip_zero base + i2c + button)\r\n");

  osKernelInitialize();

  StartHandle = osThreadNew(StartThread, NULL, &Start_attributes);
  if (StartHandle == NULL)
  {
    Error_Handler();
  }

  ControlHandle = osThreadNew(ControlThread, NULL, &ControlThread_attributes);
  if (ControlHandle == NULL)
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
  osThreadExit();
}

static void ControlThread(void *argument)
{
  uint8_t stable = 0U;
  uint8_t raw_prev = 0U;
  TickType_t change_tick = 0U;

  (void)argument;
  log_puts("[CTRL] Button monitor ready\r\n");

  for (;;)
  {
    uint8_t raw = Demo_Button_IsPressed();
    TickType_t now = xTaskGetTickCount();

    if (raw != raw_prev)
    {
      raw_prev = raw;
      change_tick = now;
    }

    if ((now - change_tick) >= pdMS_TO_TICKS(30U) && stable != raw_prev)
    {
      stable = raw_prev;
      if (stable != 0U)
      {
        Demo_Handle_ButtonPress();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20U));
  }
}

static void BSP_Config(void)
{
  LED1_Init();
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, LED1_OFF_LEVEL);
  BSP_LED_Init(LED2);
  BSP_LED_Init(LED3);
  (void)BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);
  Demo_I2C1_Init();
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

static uint8_t Demo_Button_IsPressed(void)
{
  return (BSP_PB_GetState(BUTTON_USER) != 0U) ? 1U : 0U;
}

static void Demo_I2C1_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_I2C1_FORCE_RESET();
  __NOP();
  __HAL_RCC_I2C1_RELEASE_RESET();

  DEMO_I2C_GPIO_CLK_ENABLE();
  DEMO_I2C_CLK_ENABLE();

  gpio.Pin = DEMO_I2C_SCL_PIN | DEMO_I2C_SDA_PIN;
  gpio.Mode = GPIO_MODE_AF_OD;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = DEMO_I2C_AF;
  HAL_GPIO_Init(DEMO_I2C_GPIO_PORT, &gpio);

  h_i2c1.Instance = DEMO_I2C;
  h_i2c1.Init.Timing = 0x10909CEC;
  h_i2c1.Init.OwnAddress1 = 0U;
  h_i2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  h_i2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  h_i2c1.Init.OwnAddress2 = 0U;
  h_i2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  h_i2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  h_i2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&h_i2c1) != HAL_OK)
  {
    log_puts("[I2C] init failed\r\n");
    Error_Handler();
  }
  (void)HAL_I2CEx_ConfigAnalogFilter(&h_i2c1, I2C_ANALOGFILTER_ENABLE);
  (void)HAL_I2CEx_ConfigDigitalFilter(&h_i2c1, 0U);
  log_puts("[I2C] I2C1 ready\r\n");
}

static void Demo_I2C1_Scan(void)
{
  char line[96];
  uint32_t found = 0U;

  log_puts("[I2C] scan started\r\n");
  for (uint8_t addr = 0x03U; addr <= 0x77U; addr++)
  {
    if (HAL_I2C_IsDeviceReady(&h_i2c1, (uint16_t)(addr << 1U), 2U, 30U) == HAL_OK)
    {
      (void)snprintf(line, sizeof(line), "[I2C] found 0x%02X\r\n", (unsigned int)addr);
      log_puts(line);
      found++;
    }
  }

  (void)snprintf(line, sizeof(line), "[I2C] scan done, devices=%lu\r\n", (unsigned long)found);
  log_puts(line);
}

static void Demo_Handle_ButtonPress(void)
{
  char line[96];

  s_button_press_count++;
  (void)snprintf(line, sizeof(line), "[BTN] USER pressed, count=%lu\r\n",
                 (unsigned long)s_button_press_count);
  log_puts(line);

  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, LED1_ON_LEVEL);
  vTaskDelay(pdMS_TO_TICKS(80U));
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, LED1_OFF_LEVEL);

  Demo_I2C1_Scan();
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

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  char line[96];
  const char *name = (pcTaskName != NULL) ? pcTaskName : "?";
  (void)xTask;

  (void)snprintf(line, sizeof(line), "[RTOS] Stack overflow: %s\r\n", name);
  log_puts(line);
  __disable_irq();
  for (;;)
  {
  }
}

void vApplicationMallocFailedHook(void)
{
  log_puts("[RTOS] Malloc failed\r\n");
  __disable_irq();
  for (;;)
  {
  }
}

void _init(void)
{
}
