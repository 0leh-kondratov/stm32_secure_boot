/*
 * Minimal standalone demo: 3 LEDs + UART log (LED state).
 * NUCLEO-H743ZI2 (MB1364): LED1=PB0 (green), LED2=PE1 (blue), LED3=PB14 (red).
 * UART: USART3 PD8/PD9, 115200. Flash at 0x08000000. Build: make demo.
 */
#include <stdint.h>
#include "uart_log.h"

#define RCC_BASE            (0x58024400UL)
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_AHB4ENR_GPIOBEN (1U << 1)
#define RCC_AHB4ENR_GPIOEEN (1U << 4)

#define GPIOB_BASE          (0x58020400UL)
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18U))
#define GPIOE_BASE          (0x58021000UL)
#define GPIOE_MODER         (*(volatile uint32_t *)(GPIOE_BASE + 0x00U))
#define GPIOE_BSRR          (*(volatile uint32_t *)(GPIOE_BASE + 0x18U))

/* NUCLEO-H743ZI2: LED1=PB0, LED2=PE1, LED3=PB14 */
#define LED1_PIN  0U
#define LED2_PIN  1U   /* Port E */
#define LED3_PIN  14U

void __attribute__((weak)) SysTick_Handler(void) {}

static void delay(uint32_t n)
{
    while (n--) __asm volatile("nop");
}

static void leds_init(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOEEN;
    delay(1000); /* let clock settle */
    /* PB0, PB14 output */
    GPIOB_MODER &= ~((3U << (0*2)) | (3U << (14*2)));
    GPIOB_MODER |= (1U << (0*2)) | (1U << (14*2));
    /* PE1 output */
    GPIOE_MODER &= ~(3U << (LED2_PIN * 2));
    GPIOE_MODER |= (1U << (LED2_PIN * 2));
}

/* NUCLEO LEDs are active-low: low = ON, high = OFF */
static void led1_set(int on)
{
    if (on) GPIOB_BSRR = (1U << (LED1_PIN + 16U));
    else    GPIOB_BSRR = (1U << LED1_PIN);
}
static void led2_set(int on)
{
    if (on) GPIOE_BSRR = (1U << (LED2_PIN + 16U));
    else    GPIOE_BSRR = (1U << LED2_PIN);
}
static void led3_set(int on)
{
    if (on) GPIOB_BSRR = (1U << (LED3_PIN + 16U));
    else    GPIOB_BSRR = (1U << LED3_PIN);
}

static void log_led_state(int l1, int l2, int l3)
{
    log_puts("LED1=");
    log_puts(l1 ? "ON " : "OFF ");
    log_puts("LED2=");
    log_puts(l2 ? "ON " : "OFF ");
    log_puts("LED3=");
    log_puts(l3 ? "ON\r\n" : "OFF\r\n");
}

int main(void)
{
    leds_init();
    /* One quick blink before UART so we see activity even if log_init fails */
    led1_set(1);
    delay(200000U);
    led1_set(0);
    delay(200000U);

    log_init();
    log_puts("Demo: LEDs + log (NUCLEO-H743ZI2)\r\n");

    log_led_state(0, 0, 0);

    for (;;) {
        led1_set(1);
        led2_set(0);
        led3_set(0);
        log_led_state(1, 0, 0);
        delay(600000U);

        led1_set(0);
        led2_set(1);
        led3_set(0);
        log_led_state(0, 1, 0);
        delay(600000U);

        led1_set(0);
        led2_set(0);
        led3_set(1);
        log_led_state(0, 0, 1);
        delay(600000U);
    }
    return 0;
}
