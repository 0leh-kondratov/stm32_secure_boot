/*
 * Demo with FreeRTOS: 2 tasks blink LED1/LED2 + интерактивный шелл по UART.
 * Команды: help, led1 on|off, led2 on|off, led3 on|off, status.
 */
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "uart_log.h"

#define RCC_BASE            (0x58024400UL)
#define RCC_AHB4ENR         (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_AHB4ENR_GPIOBEN (1U << 1)
#define RCC_AHB4ENR_GPIOEEN (1U << 4)

#define GPIOB_BASE          (0x58020400UL)
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18U))
#define GPIOB_ODR           (*(volatile uint32_t *)(GPIOB_BASE + 0x14U))
#define GPIOE_BASE          (0x58021000UL)
#define GPIOE_MODER         (*(volatile uint32_t *)(GPIOE_BASE + 0x00U))
#define GPIOE_BSRR          (*(volatile uint32_t *)(GPIOE_BASE + 0x18U))
#define GPIOE_ODR           (*(volatile uint32_t *)(GPIOE_BASE + 0x14U))

#define LED1_PIN  0U
#define LED2_PIN  1U
#define LED3_PIN  14U

#define LINE_MAX  48

static volatile int led1_manual, led2_manual;
static volatile int led1_override, led2_override;

#ifndef DEMO_USE_SYSTEM_CLOCK
uint32_t SystemCoreClock = 64000000UL;
#else
extern uint32_t SystemCoreClock;
#endif

extern void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
    xPortSysTickHandler();
}

static void leds_init(void)
{
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOEEN;
    GPIOB_MODER &= ~((3U << (0*2)) | (3U << (14*2)));
    GPIOB_MODER |= (1U << (0*2)) | (1U << (14*2));
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

static void task_led1(void *pv)
{
    (void)pv;
    for (;;) {
        if (led1_manual)
            led1_set(led1_override);
        else {
            led1_set(1);
            vTaskDelay(pdMS_TO_TICKS(200));
            led1_set(0);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void task_led2(void *pv)
{
    (void)pv;
    for (;;) {
        if (led2_manual)
            led2_set(led2_override);
        else {
            led2_set(1);
            vTaskDelay(pdMS_TO_TICKS(400));
            led2_set(0);
            vTaskDelay(pdMS_TO_TICKS(400));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static int str_prefix(const char *line, const char *cmd)
{
    size_t n = 0;
    while (cmd[n] && line[n] == cmd[n]) n++;
    if (cmd[n] != '\0') return 0;
    if (line[n] == '\0' || line[n] == ' ' || line[n] == '\r' || line[n] == '\n') return 1;
    return 0;
}

/* После префикса "ledX" пропустить пробелы и проверить "on"/"off". Возвращает 1=on, 0=off, -1=нет. */
static int led_arg(const char *p)
{
    while (*p == ' ' || *p == '\t') p++;
    if (str_prefix(p, "on"))  return 1;
    if (str_prefix(p, "off")) return 0;
    return -1;
}

static void run_cmd(char *line)
{
    size_t i = 0;
    while (line[i] && line[i] != '\r' && line[i] != '\n') i++;
    line[i] = '\0';
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
    if (!*line) return;

    if (str_prefix(line, "help")) {
        log_puts("help, led1/led2/led3 on|off, status\r\n");
        return;
    }
    if (str_prefix(line, "status")) {
        /* active-low: ODR 0 = LED on */
        log_puts("LED1="); log_puts((GPIOB_ODR & (1U << LED1_PIN)) ? "OFF " : "ON ");
        log_puts(" LED2="); log_puts((GPIOE_ODR & (1U << LED2_PIN)) ? "OFF " : "ON ");
        log_puts(" LED3="); log_puts((GPIOB_ODR & (1U << LED3_PIN)) ? "OFF\r\n" : "ON\r\n");
        return;
    }
    if (str_prefix(line, "led1")) {
        int a = led_arg(line + 4);
        if (a >= 0) { led1_manual = 1; led1_override = a; log_puts(a ? "LED1 on\r\n" : "LED1 off\r\n"); return; }
        led1_manual = 0; log_puts("LED1 auto\r\n"); return;
    }
    if (str_prefix(line, "led2")) {
        int a = led_arg(line + 4);
        if (a >= 0) { led2_manual = 1; led2_override = a; log_puts(a ? "LED2 on\r\n" : "LED2 off\r\n"); return; }
        led2_manual = 0; log_puts("LED2 auto\r\n"); return;
    }
    if (str_prefix(line, "led3")) {
        int a = led_arg(line + 4);
        if (a >= 0) { led3_set(a); log_puts(a ? "LED3 on\r\n" : "LED3 off\r\n"); return; }
        log_puts("led3 on|off\r\n"); return;
    }
    log_puts("? type help\r\n");
}

static void task_shell(void *pv)
{
    char buf[LINE_MAX];
    int len = 0;
    (void)pv;
    /* Приглашение уже выведено из main(); здесь только цикл ввода */
    for (;;) {
        int c = log_getchar();
        if (c >= 0) {
            if (c == '\r' || c == '\n') {
                log_puts("\r\n");
                if (len > 0) { buf[len] = '\0'; run_cmd(buf); len = 0; }
                log_puts("> ");
            } else if (len < (int)(sizeof(buf) - 1)) {
                buf[len++] = (char)c;
                if (c >= 32 && c < 127) { char e[2] = { (char)c, 0 }; log_puts(e); }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

#ifdef DEMO_USE_SYSTEM_CLOCK
extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);
#endif

int main(void)
{
#ifdef DEMO_USE_SYSTEM_CLOCK
    SystemInit();
    SystemCoreClockUpdate();
#endif
    leds_init();
    led3_set(1);
    log_init();
    log_puts("Demo UART OK\r\n");
    log_puts("\r\nFreeRTOS interactive. Type 'help'.\r\n> ");

    xTaskCreate(task_led1, "L1", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    xTaskCreate(task_led2, "L2", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    xTaskCreate(task_shell, "SH", configMINIMAL_STACK_SIZE * 6, NULL, 2, NULL);

    vTaskStartScheduler();
    for (;;) { }
    return 0;
}
