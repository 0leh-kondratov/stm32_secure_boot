/**
  ******************************************************************************
  * @file    main.c
  * @brief   Step2 — FreeRTOS: Logger, LED blink, "Step 2: OK", I2C scanner to UART log.
  *          I2C init only in task (first scan); uses step1 linker/startup.
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
#ifndef STEP2_RENODE
static void CPU_CACHE_Enable(void);
#endif
static void LED1_Init(void);
static void MX_I2C1_Init(void);

UART_HandleTypeDef UartHandle;
I2C_HandleTypeDef hi2c1;

QueueHandle_t xLogQueue;

#define LOG_QUEUE_LEN    6
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

/* Helper: send one log line to queue (buffer must remain valid until consumed) */
static void log_send(char *buf)
{
  char *pBuf = buf;
  (void)xQueueSend(xLogQueue, &pBuf, pdMS_TO_TICKS(100));
}

/* I2C scan; init I2C on first call only (in task, not in main). */
static void do_i2c_scan(void)
{
  static char buf[LOG_MSG_MAX_LEN];
  static uint8_t i2c_inited = 0;
  const char hex[] = "0123456789ABCDEF";
  int found = 0;

  if (!i2c_inited)
  {
    MX_I2C1_Init();
    i2c_inited = 1;
  }

#if I2Cx_USE_PB8_PB9
  (void)snprintf(buf, sizeof(buf), "Step 2: I2C scan (I2C1 PB8 SCL, PB9 SDA)...\r\n");
#else
  (void)snprintf(buf, sizeof(buf), "Step 2: I2C scan (I2C1 PB6 SCL, PB7 SDA)...\r\n");
#endif
  log_send(buf);
  vTaskDelay(pdMS_TO_TICKS(25));

  for (uint8_t addr = 0x03; addr <= 0x77; addr++)
  {
    uint8_t dev_addr = addr << 1;
    if (HAL_I2C_IsDeviceReady(&hi2c1, dev_addr, 2, 50) == HAL_OK)
    {
      (void)snprintf(buf, sizeof(buf), "  [FOUND] 0x%c%c (7-bit)\r\n",
                    hex[addr >> 4], hex[addr & 0x0F]);
      log_send(buf);
      vTaskDelay(pdMS_TO_TICKS(25));
      found++;
    }
  }

  if (found == 0)
  {
    (void)snprintf(buf, sizeof(buf), "  No I2C device found.\r\n");
    log_send(buf);
  }
  else
  {
    (void)snprintf(buf, sizeof(buf), "Total: %d device(s).\r\n", found);
    log_send(buf);
  }
}

/* App Task: "Step 2: OK" every 2 s; I2C scan every 5 s (first scan after 5 s) */
static void vAppTask(void *pvParameters)
{
  static char buf[LOG_MSG_MAX_LEN];
  char *pBuf = buf;
  TickType_t last_scan = 0;
  (void)pvParameters;

  for (;;)
  {
    TickType_t now = xTaskGetTickCount();
    if ((now - last_scan) >= pdMS_TO_TICKS(5000))
    {
      do_i2c_scan();
      last_scan = now;
    }

    (void)snprintf(buf, sizeof(buf), "Step 2: OK\r\n");
    if (xQueueSend(xLogQueue, &pBuf, pdMS_TO_TICKS(100)) != pdTRUE) { /* skip if full */ }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

/* Called from SysTick; advance HAL timebase */
void vApplicationTickHook(void)
{
  HAL_IncTick();
}

int main(void)
{
  /* Very first: light LED1 (PB0) via registers (no HAL). */
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
  __DSB();
  __ISB();
  GPIOB->MODER = (GPIOB->MODER & ~(3U << 0)) | (1U << 0);  /* PB0 = output */
  GPIOB->ODR &= ~(1U << 0);  /* PB0 low = LED on (active-low) */

  MPU_Config();
#ifdef STEP2_RENODE
  (void)0; /* no cache in Renode */
#endif

  HAL_Init();

  /* Re-init LED1 properly (HAL), keep on */
  LED1_Init();
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);

  SystemClock_Config();
#ifndef STEP2_RENODE
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

static void MX_I2C1_Init(void)
{
  GPIO_InitTypeDef g = {0};

/* Zresetuj I2C przed inicjalizacją (w przypadku zablokowania magistrali po zresetowaniu płyty) */
  __HAL_RCC_I2C1_FORCE_RESET();
  for (volatile int i = 0; i < 100; i++) { (void)i; }
  __HAL_RCC_I2C1_RELEASE_RESET();

  I2Cx_GPIO_CLK_ENABLE();
  I2Cx_CLK_ENABLE();

  g.Pin       = I2Cx_SCL_PIN | I2Cx_SDA_PIN;
  g.Mode      = GPIO_MODE_AF_OD;
#if I2Cx_GPIO_PULLUP
  g.Pull      = GPIO_PULLUP;
#else
  g.Pull      = GPIO_NOPULL;
#endif
  g.Speed     = GPIO_SPEED_FREQ_LOW;
  g.Alternate = I2Cx_AF;
  HAL_GPIO_Init(I2Cx_GPIO_PORT, &g);

  hi2c1.Instance             = I2Cx;
/* Taktowanie: 0x10909CEC ≈ 100 kHz przy 200 MHz APB1; przy 100 MHz APB1 otrzymujesz ~50 kHz - obie opcje obowiązują dla I2C */
  hi2c1.Init.Timing          = 0x10909CEC;
  hi2c1.Init.OwnAddress1     = 0;
  hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2     = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    Error_Handler();
  (void)HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
  (void)HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}

static void SystemClock_Config(void)
{
#ifdef STEP2_RENODE
  /* Renode has no HSE; keep default HSI 64 MHz, no PLL. */
  return;
#endif
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  HAL_StatusTypeDef ret = HAL_OK;

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
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

#ifndef STEP2_RENODE
static void CPU_CACHE_Enable(void)
{
  SCB_EnableICache();
  SCB_EnableDCache();
}
#endif

static void MPU_Config(void)
{
  HAL_MPU_Disable();
}

void Error_Handler(void)
{
  for (;;) {}
}
