/*
 * Minimal bootloader: LED1 on + UART log test.
 * Use to verify that code runs (LED on) and UART works (115200 8N1, USART3 PD8/PD9).
 * Build: make minimal-usart3
 * Flash: st-flash write build/bootloader_minimal_usart3.bin 0x08000000
 */
#include <stdint.h>
#include "uart_log.h"

#define RCC_BASE           (0x58024400UL)
#define RCC_AHB4ENR        (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_AHB4ENR_GPIOB  (1U << 1)
#define GPIOB_BASE         (0x58020400UL)
#define GPIOB_MODER        (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_BSRR         (*(volatile uint32_t *)(GPIOB_BASE + 0x18U))
#define LED1_PIN           (0U)

int main(void)
{
    /* LED1 on first so we see activity even if UART fails */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOB;
    GPIOB_MODER = (GPIOB_MODER & ~(3U << (LED1_PIN * 2U))) | (1U << (LED1_PIN * 2U));
    GPIOB_BSRR = (1U << LED1_PIN);

    log_init();
    log_puts("\r\n=== Bootloader log test ===\r\n");
    log_puts("If you see this, UART is OK.\r\n");
    log_puts("115200 8N1\r\n");

    for (;;) {
        __asm volatile("wfi");
    }
    return 0;
}
