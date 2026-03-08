/*
 * Minimal application for Secure Boot demo.
 * Blinks LD1 (green, PB0) on NUCLEO-144.
 * Vector table must be at 0x08010000 + sizeof(image_header_t).
 * This project is built as a raw .bin of .text+.data; the sign_image.py script
 * prepends image_header_t and writes the combined image to flash at 0x08010000.
 */
#include <stdint.h>

#define RCC_BASE            (0x58024400UL)
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_AHB4ENR_GPIOBEN (1U << 1)

#define GPIOB_BASE          (0x58020400UL)
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18U))
#define LED1_PIN            (0U)

extern uint32_t _estack;

void __attribute__((weak)) SysTick_Handler(void) {}

static void delay(uint32_t count)
{
    while (count--) __asm volatile("nop");
}

int main(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    GPIOB_MODER &= ~(3U << (LED1_PIN * 2U));
    GPIOB_MODER |= (1U << (LED1_PIN * 2U));

    for (;;) {
        GPIOB_BSRR = (1U << LED1_PIN);
        delay(500000U);
        GPIOB_BSRR = (1U << (LED1_PIN + 16U));
        delay(500000U);
    }
    return 0;
}
