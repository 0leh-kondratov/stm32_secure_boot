/**
  ******************************************************************************
  * @file    main.c
  * @brief   Step1 — FreeRTOS: Logger task (xLogQueue + HAL_UART_Transmit),
  *          LED task (blink 500 ms), App task (sends "Step 1: OK" to queue).
  ******************************************************************************
  */

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

static void MPU_Config(void);
static void SystemClock_Config(void);
#ifndef STEP1_RENODE
static void CPU_CACHE_Enable(void);
#endif
static void LED1_Init(void);

UART_HandleTypeDef UartHandle;

QueueHandle_t xLogQueue;

#define LOG_QUEUE_LEN    4
#define LOG_MSG_MAX_LEN  128
#define UART_TX_TIMEOUT_MS  500

/* Logger Task: wait for string pointers from xLogQueue, output via HAL_UART_Transmit */
static void vLoggerTask(void *pvParameters)
{
  char *pMsg;
  (void)pvParameters;

  for (;;)
  {
    if (xQueueReceive(xLogQueue, &pMsg, portMAX_DELAY) == pdTRUE)
    {
      if (pMsg != NULL)
      {
        size_t len = strlen(pMsg);
        if (len > 0 && len < LOG_MSG_MAX_LEN)
        {
          HAL_UART_Transmit(&UartHandle, (uint8_t *)pMsg, (uint16_t)len, UART_TX_TIMEOUT_MS);
        }
      }
    }
  }
}

/* LED Task: blink LED1 every 500 ms */
static void vLedTask(void *pvParameters)
{
  (void)pvParameters;

  for (;;)
  {
    HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/* App Task: replaces former while(1) — format message and send pointer to xLogQueue */
static void vAppTask(void *pvParameters)
{
  static char buf[LOG_MSG_MAX_LEN];
  char *pBuf = buf;
  (void)pvParameters;

  for (;;)
  {
    (void)snprintf(buf, sizeof(buf), "Step 1: OK\r\n");
    if (xQueueSend(xLogQueue, &pBuf, pdMS_TO_TICKS(100)) != pdTRUE)
    {
      /* queue full, skip or retry later */
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

/* Called from SysTick; advance HAL timebase so HAL_Delay etc. work */
void vApplicationTickHook(void)
{
  HAL_IncTick();
}

int main(void)
{
  /* Very first: light LED1 (PB0) via registers (no HAL). If this never lights, we don't reach main() or pin is wrong. */
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
  __DSB();
  __ISB();
  GPIOB->MODER = (GPIOB->MODER & ~(3U << 0)) | (1U << 0);  /* PB0 = output */
  GPIOB->ODR &= ~(1U << 0);  /* PB0 low = LED on (active-low) */

  MPU_Config();
#ifdef STEP1_RENODE
  (void)0; /* no cache in Renode */
#endif

  HAL_Init();

  /* Re-init LED1 properly (HAL), keep on */
  LED1_Init();
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);

  SystemClock_Config();
#ifndef STEP1_RENODE
  SystemCoreClockUpdate();
  CPU_CACHE_Enable(); /* after clock switch — recommended for H7 */
#else
  SystemCoreClockUpdate();
  SystemCoreClock = 64000000UL; /* Renode: no HSE, force 64 MHz for FreeRTOS tick */
#endif

  UartHandle.Instance        = USARTx;
  UartHandle.Init.BaudRate   = 115200;
  UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits   = UART_STOPBITS_1;
  UartHandle.Init.Parity     = UART_PARITY_NONE;
  UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode       = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  UartHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&UartHandle) != HAL_OK)
  {
    Error_Handler();
  }

  xLogQueue = xQueueCreate(LOG_QUEUE_LEN, sizeof(char *));
  if (xLogQueue == NULL)
  {
    Error_Handler();
  }

  BaseType_t ok;
  ok = xTaskCreate(vLoggerTask, "Logger", 384, NULL, 1, NULL);
  if (ok != pdPASS) Error_Handler();

  ok = xTaskCreate(vLedTask, "LED", 192, NULL, 1, NULL);
  if (ok != pdPASS) Error_Handler();

  ok = xTaskCreate(vAppTask, "App", 384, NULL, 2, NULL);
  if (ok != pdPASS) Error_Handler();

  vTaskStartScheduler();

  for (;;) {}
}

static void LED1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  LED1_GPIO_CLK_ENABLE();
  GPIO_InitStruct.Pin   = LED1_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED1_GPIO_PORT, &GPIO_InitStruct);
}

static void SystemClock_Config(void)
{
#ifdef STEP1_RENODE
  /* Renode has no HSE; keep default HSI 64 MHz, no PLL. */
  return;
#endif
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  HAL_StatusTypeDef ret = HAL_OK;

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
/* NUCLEO-H743ZI2: domyślny HSE z MCO ST-Link (8 MHz) to BYPASS. Jeśli na OSC jest kwarc, zmień na RCC_HSE_ON. */
  RCC_OscInitStruct.HSEState       = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState       = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState       = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM      = 4;
  RCC_OscInitStruct.PLL.PLLN      = 400;
  RCC_OscInitStruct.PLL.PLLFRACN  = 0;
  RCC_OscInitStruct.PLL.PLLP      = 2;
  RCC_OscInitStruct.PLL.PLLR      = 2;
  RCC_OscInitStruct.PLL.PLLQ      = 4;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE    = RCC_PLL1VCIRANGE_1;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if (ret != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK    |
                                 RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if (ret != HAL_OK) Error_Handler();
}

#ifndef STEP1_RENODE
static void CPU_CACHE_Enable(void)
{
  SCB_EnableICache();
  SCB_EnableDCache();
}
#endif

static void MPU_Config(void)
{
  /* MPU disabled — one region with NO_ACCESS on 4GB would block all memory and cause fault. */
  HAL_MPU_Disable();
}

void Error_Handler(void)
{
  for (;;) {}
}
