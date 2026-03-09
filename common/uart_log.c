/*
 * UART log: 115200 8N1 for ST-Link VCP on NUCLEO-144.
 * Use USART2 (PA2=TX, PA3=RX) — typical for ST-Link VCP on many Nucleo boards.
 * If no output, try building with -DUART_LOG_USE_USART3 for PD8/PD9.
 */
#include <stdint.h>
#include "uart_log.h"
#ifdef UART_LOG_FREERTOS_YIELD
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef USE_HAL_DRIVER
#include "stm32h7xx_hal.h"
#endif

#define RCC_BASE            (0x58024400UL)
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0x58U))
#define RCC_D2CFGR          (*(volatile uint32_t *)(RCC_BASE + 0x94U))
#define RCC_D2CCIP2R        (*(volatile uint32_t *)(RCC_BASE + 0x54U))

#define USART_CR1_UE        (1U << 0)
#define USART_CR1_TE        (1U << 3)
#define USART_CR1_RE        (1U << 2)
#define USART_ISR_TXE       (1U << 7)
#define USART_ISR_RXNE      (1U << 5)

/* BRR is computed at runtime when HAL is available. */
#define UART_BRR_FALLBACK_115200_AT_64MHZ (35U)

#if defined(UART_LOG_USE_USART3)
/* USART3, PD8 (TX), PD9 (RX) */
#define RCC_AHB4ENR_GPIOXEN (1U << 3)
#define GPIOX_BASE          (0x58020C00UL)
#define GPIOX_MODER         (*(volatile uint32_t *)(GPIOX_BASE + 0x00U))
#define GPIOX_AFR1          (*(volatile uint32_t *)(GPIOX_BASE + 0x24U))
#define RCC_APB1LENR_UARTEN (1U << 18)
#define UART_BASE           (0x40004800UL)
#define MODER_MASK          ((3U << (8*2)) | (3U << (9*2)))
#define MODER_ALT           ((2U << (8*2)) | (2U << (9*2)))
#define AFR_MASK            (0xFFU)
#define AFR_VAL             (0x77U)  /* AF7 for PD8 and PD9 */
#else
/* USART2, PA2 (TX), PA3 (RX) */
#define RCC_AHB4ENR_GPIOXEN (1U << 0)
#define GPIOX_BASE          (0x58020000UL)
#define GPIOX_MODER         (*(volatile uint32_t *)(GPIOX_BASE + 0x00U))
#define GPIOX_AFRL          (*(volatile uint32_t *)(GPIOX_BASE + 0x20U))
#define RCC_APB1LENR_UARTEN (1U << 17)
#define UART_BASE           (0x40004400UL)
#define MODER_MASK          ((3U << (2*2)) | (3U << (3*2)))
#define MODER_ALT           ((2U << (2*2)) | (2U << (3*2)))
#define AFR_MASK            (0xFF00U)   /* bits 8-15 for PA2, PA3 */
#define AFR_VAL             (0x7700U)   /* AF7 for PA2 and PA3 */
#endif

#define UART_CR1            (*(volatile uint32_t *)(UART_BASE + 0x00U))
#define UART_BRR            (*(volatile uint32_t *)(UART_BASE + 0x0CU))
#define UART_ISR            (*(volatile uint32_t *)(UART_BASE + 0x1CU))
#define UART_TDR            (*(volatile uint32_t *)(UART_BASE + 0x28U))
#define UART_RDR            (*(volatile uint32_t *)(UART_BASE + 0x24U))

static void delay_loop(uint32_t n)
{
    while (n--) __asm volatile("nop");
}

void log_init(void)
{
#if defined(DEMO_USE_SYSTEM_CLOCK)
    /* USART2/3 clock source = PCLK1 (0 = PCLK1). Required for correct baud on H7. */
    RCC_D2CCIP2R &= ~0x07U;
#endif
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOXEN;
    RCC_APB1LENR |= RCC_APB1LENR_UARTEN;

    GPIOX_MODER &= ~MODER_MASK;
    GPIOX_MODER |= MODER_ALT;
#if defined(UART_LOG_USE_USART3)
    GPIOX_AFR1 = (GPIOX_AFR1 & ~AFR_MASK) | AFR_VAL;
#else
    GPIOX_AFRL = (GPIOX_AFRL & ~AFR_MASK) | AFR_VAL;
#endif

    uint32_t brr = UART_BRR_FALLBACK_115200_AT_64MHZ;
#define UART_DIV_115200_16X  (16U * 115200U)  /* 1843200: BRR = PCLK1 / this */
#ifdef USE_HAL_DRIVER
    /* USART2/3 are on APB1. After SystemClock_Config(), HAL knows the real PCLK1. */
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    if (pclk1 != 0U) {
        brr = (pclk1 + (UART_DIV_115200_16X / 2U)) / UART_DIV_115200_16X;
        if (brr == 0U) brr = 1U;
    }
#elif defined(DEMO_USE_SYSTEM_CLOCK)
    /* Demo/Test with system_stm32h7xx: PCLK1 = SystemD2Clock / D2PPRE1. */
    extern uint32_t SystemD2Clock;
    uint32_t d2ppre1 = (RCC_D2CFGR >> 8U) & 7U;
    if (d2ppre1 > 4U) d2ppre1 = 4U;
    uint32_t pclk1 = SystemD2Clock >> d2ppre1;
    if (pclk1 != 0U) {
        brr = (pclk1 + (UART_DIV_115200_16X / 2U)) / UART_DIV_115200_16X;
        if (brr == 0U) brr = 1U;
    }
#endif
    UART_BRR = brr;
    UART_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    /* Short delay so host terminal / USB-VCP is ready after reset */
    delay_loop(500000U);
}

int log_getchar(void)
{
    if ((UART_ISR & USART_ISR_RXNE) == 0)
        return -1;
    return (int)(UART_RDR & 0xFFU);
}

/* Таймаут ожидания TXE (циклов), чтобы не зависать в Renode, если эмулятор не сбрасывает TXE */
#define LOG_PUTS_TX_TIMEOUT  50000
/* При UART_LOG_FREERTOS_YIELD при таймауте делаем vTaskDelay(1) и повторяем — даём эмулятору время и не блокируем шелл */
#ifdef UART_LOG_FREERTOS_YIELD
#define LOG_PUTS_RETRIES    150
#endif

void log_puts(const char *s)
{
    if (!s) return;
    while (*s) {
        uint32_t n = LOG_PUTS_TX_TIMEOUT;
        while ((UART_ISR & USART_ISR_TXE) == 0 && n != 0)
            n--;
#ifdef UART_LOG_FREERTOS_YIELD
        if (n == 0) {
            unsigned r = LOG_PUTS_RETRIES;
            while (r != 0 && (UART_ISR & USART_ISR_TXE) == 0) {
                vTaskDelay(pdMS_TO_TICKS(1));
                r--;
            }
            if (r == 0)
                break;
        }
#else
        if (n == 0)
            break; /* не блокируемся навсегда (например в Renode) */
#endif
        UART_TDR = (uint32_t)(unsigned char)*s++;
    }
}
