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
#endif

#define LOG_UART_TIMEOUT_MS  1000U

#ifdef USE_HAL_DRIVER

/* --- HAL: USART2 (PA2/PA3) = ST-Link VCP, or USART3 (PD8/PD9) --- */
static UART_HandleTypeDef hlog_uart;

void log_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

#if defined(UART_LOG_USE_USART2)
  /* USART2: PA2 (TX), PA3 (RX) — подключено к ST-Link VCP на NUCLEO-H743ZI2 */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();

  GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  hlog_uart.Instance = USART2;
#else
  /* USART3: PD8 (TX), PD9 (RX) — нужен внешний USB-UART */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_USART3_CLK_ENABLE();

  GPIO_InitStruct.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  hlog_uart.Instance = USART3;
#endif

  hlog_uart.Init.BaudRate      = 115200;
  hlog_uart.Init.WordLength    = UART_WORDLENGTH_8B;
  hlog_uart.Init.StopBits      = UART_STOPBITS_1;
  hlog_uart.Init.Parity        = UART_PARITY_NONE;
  hlog_uart.Init.Mode          = UART_MODE_TX_RX;
  hlog_uart.Init.HwFlowCtl     = UART_HWCONTROL_NONE;
  hlog_uart.Init.OverSampling  = UART_OVERSAMPLING_16;
  hlog_uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlog_uart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlog_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  (void)HAL_UART_Init(&hlog_uart);
}

void log_puts(const char *str)
{
  if (str == NULL || hlog_uart.gState != HAL_UART_STATE_READY)
    return;
  size_t len = strlen(str);
  if (len == 0)
    return;
  (void)HAL_UART_Transmit(&hlog_uart, (const uint8_t *)str, (uint16_t)len, LOG_UART_TIMEOUT_MS);
}

int log_getchar(void)
{
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
