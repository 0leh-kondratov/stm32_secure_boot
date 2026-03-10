/**
 * @file uart_log.c
 * @brief Robust UART logging for NUCLEO-H743ZI2 — USART3 (PD8 TX, PD9 RX), 115200 8N1.
 *
 * When USE_HAL_DRIVER is defined: uses HAL in blocking mode (HAL_UART_Transmit).
 * For accurate baud at 400 MHz SYSCLK, call log_init() after SystemClock_Config().
 * On STM32H743, USART3 is clocked by D2PCLK1 (APB1).
 */
#include <stdint.h>
#include <string.h>
#include "uart_log.h"

#ifdef USE_HAL_DRIVER
#include "stm32h7xx_hal.h"
#if defined(UART_LOG_USE_FREERTOS_QUEUE)
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#endif
#endif

#define LOG_UART_TIMEOUT_MS  20U

#ifdef USE_HAL_DRIVER

/* --- HAL: USART2 (PA2/PA3) = ST-Link VCP, or USART3 (PD8/PD9) --- */
static UART_HandleTypeDef hlog_uart;
static uint8_t hlog_uart_ready = 0U;

static HAL_StatusTypeDef log_uart_setup_instance(USART_TypeDef *instance)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (instance == USART3)
  {
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
  else if (instance == USART2)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else
  {
    return HAL_ERROR;
  }

  hlog_uart.Instance = instance;
  hlog_uart.Init.BaudRate       = 115200;
  hlog_uart.Init.WordLength     = UART_WORDLENGTH_8B;
  hlog_uart.Init.StopBits       = UART_STOPBITS_1;
  hlog_uart.Init.Parity         = UART_PARITY_NONE;
  hlog_uart.Init.Mode           = UART_MODE_TX_RX;
  hlog_uart.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
  hlog_uart.Init.OverSampling   = UART_OVERSAMPLING_16;
  hlog_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlog_uart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlog_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&hlog_uart) != HAL_OK)
  {
    return HAL_ERROR;
  }

  for (uint32_t i = 0; i < 100000U; i++)
  {
    if (__HAL_UART_GET_FLAG(&hlog_uart, UART_FLAG_TXE) != 0U)
    {
      return HAL_OK;
    }
  }

  (void)HAL_UART_DeInit(&hlog_uart);
  return HAL_TIMEOUT;
}

#if defined(UART_LOG_USE_FREERTOS_QUEUE)
#define LOG_QUEUE_LEN      4U
#define LOG_MSG_MAX_LEN    128U
#define LOG_TASK_STACK     384U
#define LOG_TASK_PRIO      (tskIDLE_PRIORITY + 1U)

typedef struct
{
  char text[LOG_MSG_MAX_LEN];
} log_msg_t;

static QueueHandle_t slog_queue = NULL;
static TaskHandle_t slog_task = NULL;

static void log_uart_tx_blocking(const char *str)
{
  size_t len;
  if (str == NULL || hlog_uart_ready == 0U)
  {
    return;
  }
  len = strlen(str);
  if (len == 0U)
  {
    return;
  }
  (void)HAL_UART_Transmit(&hlog_uart, (const uint8_t *)str, (uint16_t)len, LOG_UART_TIMEOUT_MS);
}

static void log_task(void *arg)
{
  log_msg_t msg;
  (void)arg;
  for (;;)
  {
    if (xQueueReceive(slog_queue, &msg, portMAX_DELAY) == pdTRUE)
    {
      log_uart_tx_blocking(msg.text);
    }
  }
}

static void log_try_start_async(void)
{
  if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
  {
    return;
  }

  if (slog_queue == NULL)
  {
    slog_queue = xQueueCreate((UBaseType_t)LOG_QUEUE_LEN, sizeof(log_msg_t));
    if (slog_queue == NULL)
    {
      return;
    }
  }

  if (slog_task == NULL)
  {
    if (xTaskCreate(log_task, "Log", (configSTACK_DEPTH_TYPE)LOG_TASK_STACK, NULL, (UBaseType_t)LOG_TASK_PRIO, &slog_task) != pdPASS)
    {
      slog_task = NULL;
    }
  }
}
#endif

void log_init(void)
{
  HAL_StatusTypeDef st = HAL_ERROR;
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  hlog_uart_ready = 0U;
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART234578;
  PeriphClkInit.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PCLK1;
  (void)HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

#if defined(UART_LOG_USE_USART2)
  st = log_uart_setup_instance(USART2);
#elif defined(UART_LOG_USE_USART3)
  st = log_uart_setup_instance(USART3);
#else
  /* Default: prefer USART3 (step1), fallback to USART2 when TXE is dead. */
  st = log_uart_setup_instance(USART3);
  if (st != HAL_OK)
  {
    st = log_uart_setup_instance(USART2);
  }
#endif

  if (st == HAL_OK)
  {
    hlog_uart_ready = 1U;
  }

}

void log_puts(const char *str)
{
#if defined(UART_LOG_USE_FREERTOS_QUEUE)
  log_msg_t msg;
  size_t n;

  if (str == NULL || hlog_uart_ready == 0U)
  {
    return;
  }

  if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
  {
    log_uart_tx_blocking(str);
    return;
  }

  log_try_start_async();

  if (slog_queue == NULL)
  {
    log_uart_tx_blocking(str);
    return;
  }

  n = strlen(str);
  if (n >= LOG_MSG_MAX_LEN)
  {
    n = LOG_MSG_MAX_LEN - 1U;
  }
  if (n == 0U)
  {
    return;
  }

  memcpy(msg.text, str, n);
  msg.text[n] = '\0';
  if (xQueueSend(slog_queue, &msg, 0U) != pdTRUE)
  {
    /* Keep critical logs even when queue is temporarily full. */
    log_uart_tx_blocking(msg.text);
  }
#else
  if (str == NULL || hlog_uart_ready == 0U || hlog_uart.gState != HAL_UART_STATE_READY)
    return;
  size_t len = strlen(str);
  if (len == 0)
    return;
  (void)HAL_UART_Transmit(&hlog_uart, (const uint8_t *)str, (uint16_t)len, LOG_UART_TIMEOUT_MS);
#endif
}

int log_getchar(void)
{
  if (hlog_uart_ready == 0U)
    return -1;
  if (__HAL_UART_GET_FLAG(&hlog_uart, UART_FLAG_RXNE) == 0U)
    return -1;
  return (int)(uint8_t)(hlog_uart.Instance->RDR & 0xFFU);
}

#else

/* --- Non-HAL fallback (e.g. demo without HAL): USART3 PD8/PD9, register access --- */
#define RCC_AHB4ENR    (*(volatile uint32_t *)(0x58024400U + 0xE0U))
#define RCC_APB1LENR   (*(volatile uint32_t *)(0x58024400U + 0x58U))
#define GPIOD_MODER    (*(volatile uint32_t *)(0x58020C00U + 0x00U))
#define GPIOD_AFR1     (*(volatile uint32_t *)(0x58020C00U + 0x24U))
#define USART3_BASE    0x40004800U
#define USART_BRR      (*(volatile uint32_t *)(USART3_BASE + 0x0CU))
#define USART_CR1      (*(volatile uint32_t *)(USART3_BASE + 0x00U))
#define USART_ISR      (*(volatile uint32_t *)(USART3_BASE + 0x1CU))
#define USART_TDR      (*(volatile uint32_t *)(USART3_BASE + 0x28U))
#define USART_RDR      (*(volatile uint32_t *)(USART3_BASE + 0x24U))
#define USART_ISR_TXE  (1U << 7)
#define USART_ISR_RXNE (1U << 5)
#define USART_CR1_UE   (1U << 0)
#define USART_CR1_TE   (1U << 3)
#define USART_CR1_RE   (1U << 2)

static uint32_t log_pclk1(void); /* forward */

void log_init(void)
{
  RCC_AHB4ENR |= (1U << 3);   /* GPIOD */
  RCC_APB1LENR |= (1U << 18); /* USART3 */
  GPIOD_MODER = (GPIOD_MODER & ~((3U << (8*2)) | (3U << (9*2)))) | ((2U << (8*2)) | (2U << (9*2)));
  GPIOD_AFR1  = (GPIOD_AFR1 & ~0xFFU) | 0x77U; /* AF7 for PD8, PD9 */
  uint32_t pclk = log_pclk1();
  if (pclk == 0U) pclk = 64000000U;
  USART_BRR = (pclk + (115200U * 8U)) / (115200U * 16U);
  if (USART_BRR == 0U) USART_BRR = 1U;
  USART_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void log_puts(const char *str)
{
  if (!str) return;
  while (*str) {
    uint32_t n = 100000U;
    while ((USART_ISR & USART_ISR_TXE) == 0U && n != 0U) n--;
    if (n == 0U) break;
    USART_TDR = (uint32_t)(unsigned char)*str++;
  }
}

int log_getchar(void)
{
  if ((USART_ISR & USART_ISR_RXNE) == 0U)
    return -1;
  return (int)(USART_RDR & 0xFFU);
}

#if defined(DEMO_USE_SYSTEM_CLOCK)
extern uint32_t SystemD2Clock;
static uint32_t log_pclk1(void)
{
  uint32_t d2ppre1 = (*(volatile uint32_t *)(0x58024400U + 0x94U) >> 8U) & 7U;
  if (d2ppre1 > 4U) d2ppre1 = 4U;
  return SystemD2Clock >> d2ppre1;
}
#else
static uint32_t log_pclk1(void) { return 0U; }
#endif

#endif /* USE_HAL_DRIVER */

__attribute__((weak)) void log_ip_address(struct netif *netif)
{
  (void)netif;
}
