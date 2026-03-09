/*
 * UART log: 115200 8N1 for ST-Link VCP on NUCLEO-144.
 * Use USART2 (PA2=TX, PA3=RX) — typical for ST-Link VCP on many Nucleo boards.
 * If no output, try building with -DUART_LOG_USE_USART3 for PD8/PD9.
 */
#include <stdint.h>
#include "uart_log.h"

#ifdef USE_HAL_DRIVER
#include "stm32h7xx_hal.h"
#endif

#define RCC_BASE            (0x58024400UL)
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_APB1LENR        (*(volatile uint32_t *)(RCC_BASE + 0x58U))

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
#ifdef USE_HAL_DRIVER
    /* USART2/3 are on APB1. After SystemClock_Config(), HAL knows the real PCLK1. */
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    if (pclk1 != 0U) {
        brr = (pclk1 + (115200U / 2U)) / 115200U; /* rounded */
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

void log_puts(const char *s)
{
    if (!s) return;
    while (*s) {
        while ((UART_ISR & USART_ISR_TXE) == 0)
            ;
        UART_TDR = (uint32_t)(unsigned char)*s++;
    }
}
