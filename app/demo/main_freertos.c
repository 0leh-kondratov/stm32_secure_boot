/*
 * Demo with FreeRTOS: переключение LED1 → LED2 → LED3 → по кругу, 1 раз в 1 с.
 * Без приглашения и команд — только цикл и одна строка в UART при старте.
 * NUCLEO-H743ZI2 (MB1364): LED1=PB0, LED2=PE1, LED3=PB14 (управляет демо).
 * LD4 (ST-Link, не управляется МК):
 *   Красный  — связь с ПК есть, программатор ещё не задействован (нет прошивки/отладки в IDE).
 *   Зелёный  — прошивка успешно завершена или сессия отладки активна (ожидание).
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
#define CYCLE_DELAY_MS_MIN  1000
#define CYCLE_DELAY_MS_MAX  10000

static volatile unsigned cycle_delay_ms = 1000;
static volatile int led3_manual;
static volatile int led3_override;

#if defined(DEMO_RENODE_AUTO_CMD) || !defined(DEMO_USE_SYSTEM_CLOCK)
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

/* Медленный цикл: LED1 → LED2 → LED3 → LED1 … (один горит, остальные погашены) */
#ifdef DEMO_RENODE_AUTO_CMD
/* В Renode SysTick часто не тикает — vTaskDelay не возвращается. Busy-wait ~1 c при 64 MHz. */
static void delay_approx_1s(void)
{
    for (volatile uint32_t i = 0; i < 16000000UL; i++) { (void)i; }
}
#endif

static void task_led_cycle(void *pv)
{
    int cur = 0;
    unsigned dms = 1000;
    (void)pv;
    log_puts("[DBG] LED task running\r\n");
    for (;;) {
        dms = cycle_delay_ms;
        if (dms < CYCLE_DELAY_MS_MIN) dms = CYCLE_DELAY_MS_MIN;
        if (dms > CYCLE_DELAY_MS_MAX) dms = CYCLE_DELAY_MS_MAX;
        led1_set(cur == 0);
        led2_set(cur == 1);
        if (led3_manual)
            led3_set(led3_override);
        else
            led3_set(cur == 2);
        /* Отладка: какой LED горит (L1/L2/L3) */
        if (cur == 0) log_puts("[LED] LED1 on\r\n");
        else if (cur == 1) log_puts("[LED] LED2 on\r\n");
        else log_puts("[LED] LED3 on\r\n");
#ifdef DEMO_RENODE_AUTO_CMD
        delay_approx_1s();
#else
        vTaskDelay(pdMS_TO_TICKS(dms));
#endif
        cur = (cur + 1) % 3;
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
#ifdef DEMO_RENODE_AUTO_CMD
    if (p[0] >= '0' && p[0] <= '9' && (p[1] == ' ' || p[1] == '\t' || !p[1])) p += 2;
    while (*p == ' ' || *p == '\t') p++;
#endif
    if (str_prefix(p, "on"))  return 1;
    if (str_prefix(p, "off")) return 0;
#ifdef DEMO_RENODE_AUTO_CMD
    if (*p == 'n' || *p == 'o') return 1; /* on → "n"/"o" при потере каждого 2-го символа */
    if (*p == 'f') return 0;
#endif
    return -1;
}

static void run_cmd(char *line)
{
    size_t i = 0;
    while (line[i] && line[i] != '\r' && line[i] != '\n') i++;
    line[i] = '\0';
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
    if (!*line) return;

    if (str_prefix(line, "help")
#ifdef DEMO_RENODE_AUTO_CMD
        || str_prefix(line, "hlp")
#endif
        ) {
        log_puts("help, led3 on|off, status, speed N (1-10 s)\r\n");
        return;
    }
    if (str_prefix(line, "status")
#ifdef DEMO_RENODE_AUTO_CMD
        || str_prefix(line, "stts")
#endif
        ) {
        log_puts("LED1="); log_puts((GPIOB_ODR & (1U << LED1_PIN)) ? "OFF " : "ON ");
        log_puts(" LED2="); log_puts((GPIOE_ODR & (1U << LED2_PIN)) ? "OFF " : "ON ");
        log_puts(" LED3="); log_puts((GPIOB_ODR & (1U << LED3_PIN)) ? "OFF\r\n" : "ON\r\n");
        return;
    }
    if (str_prefix(line, "speed")
#ifdef DEMO_RENODE_AUTO_CMD
        || str_prefix(line, "spd")
#endif
        ) {
        const char *arg = line + 5;
#ifdef DEMO_RENODE_AUTO_CMD
        if (str_prefix(line, "spd")) arg = line + 3;
#endif
        while (*arg == ' ' || *arg == '\t') arg++;
        if (*arg >= '1' && *arg <= '9') {
            unsigned n = (unsigned)(*arg - '0');
            if (arg[1] >= '0' && arg[1] <= '9') n = n * 10 + (unsigned)(arg[1] - '0');
            if (n >= 1 && n <= 10) {
                cycle_delay_ms = n * 1000;
                if (n >= 10) log_puts("speed 10");
                else { char e[2] = { (char)('0' + n), 0 }; log_puts("speed "); log_puts(e); }
                log_puts(" s\r\n");
                return;
            }
        }
        log_puts("speed 1..10 (seconds per LED)\r\n");
        return;
    }
    if (str_prefix(line, "led3")
#ifdef DEMO_RENODE_AUTO_CMD
        || str_prefix(line, "ld3")
#endif
        ) {
        const char *arg = line + 4;
#ifdef DEMO_RENODE_AUTO_CMD
        if (str_prefix(line, "ld3")) arg = line + 3;
#endif
        int a = led_arg(arg);
        if (a >= 0) {
            led3_manual = 1;
            led3_override = a;
            led3_set(a);
            log_puts(a ? "LED3 on (fixed)\r\n" : "LED3 off (fixed)\r\n");
            return;
        }
        led3_manual = 0;
        log_puts("LED3 in cycle\r\n");
        return;
    }
    log_puts("? type help\r\n");
}

#ifdef DEMO_RENODE_AUTO_CMD
/* В Renode: раз в 5 с выводим status (цикл LED уже крутится в task_led_cycle). */
static void task_auto_cmd(void *pv)
{
    (void)pv;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        { char line[16]; strcpy(line, "status"); run_cmd(line); }
        log_puts("> ");
    }
}
#endif

#define SHELL_IDLE_MS  400

static void task_shell(void *pv)
{
    char buf[LINE_MAX];
    int len = 0;
    (void)pv;
#ifdef DEMO_RENODE_AUTO_CMD
    TickType_t last_char = 0;
#endif
    for (;;) {
        int c = log_getchar();
        if (c >= 0) {
#ifdef DEMO_RENODE_AUTO_CMD
            last_char = xTaskGetTickCount();
#endif
            if (c == '\r' || c == '\n') {
                log_puts("\r\n");
                if (len > 0) { buf[len] = '\0'; run_cmd(buf); len = 0; }
                log_puts("> ");
            } else if (len < (int)(sizeof(buf) - 1)) {
                buf[len++] = (char)c;
                if (c >= 32 && c < 127) { char e[2] = { (char)c, 0 }; log_puts(e); }
            }
        }
#ifdef DEMO_RENODE_AUTO_CMD
        /* В Renode \r/\n часто теряются (UART #487). Отправка по таймауту: пауза ~400 мс = ввод команды завершён */
        if (len > 0 && (xTaskGetTickCount() - last_char) >= pdMS_TO_TICKS(SHELL_IDLE_MS)) {
            buf[len] = '\0';
            log_puts("\r\n");
            run_cmd(buf);
            len = 0;
            log_puts("> ");
            last_char = xTaskGetTickCount();
        }
#endif
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

#ifdef DEMO_USE_SYSTEM_CLOCK
extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);
#endif

int main(void)
{
    /* Ранний UART и сразу LED — видно, что дошли до main, даже если дальше зависаем */
    log_init();
    leds_init();
    led1_set(1);
    led2_set(0);
    led3_set(0);
    log_puts("[DBG] Demo start\r\n");
#if defined(DEMO_USE_SYSTEM_CLOCK) && !defined(DEMO_RENODE_AUTO_CMD)
    SystemCoreClock = 64000000UL;
    log_puts("[DBG] clock ok\r\n");
#elif defined(DEMO_RENODE_AUTO_CMD)
    log_puts("[DBG] clock 64M (renode)\r\n");
#endif
    log_puts("[DBG] GPIO ok\r\n");
    log_puts("[DBG] Demo OK\r\n");
    log_puts("[DBG] creating LED task...\r\n");

    BaseType_t cr;
#ifdef DEMO_RENODE_AUTO_CMD
    /* Renode: статический стек и TCB в .bss — обходим зависание xTaskCreate (pvPortMalloc/куча в эмуляторе) */
    static StackType_t led_stack[configMINIMAL_STACK_SIZE * 4];
    static StaticTask_t led_tcb;
    log_puts("[DBG] before xTaskCreateStatic\r\n");
    TaskHandle_t h = xTaskCreateStatic(task_led_cycle, "LED", configMINIMAL_STACK_SIZE * 4, NULL, 1, led_stack, &led_tcb);
    log_puts("[DBG] after xTaskCreateStatic\r\n");
    cr = (h != NULL) ? pdPASS : pdFAIL;
#else
    cr = xTaskCreate(task_led_cycle, "LED", configMINIMAL_STACK_SIZE * 4, NULL, 1, NULL);
#endif
    log_puts("[DBG] xTaskCreate done\r\n");
    if (cr != pdPASS) {
        log_puts("[DBG] ERR: LED task FAIL\r\n");
    } else {
        log_puts("[DBG] LED task ok\r\n");
    }
    log_puts("[DBG] start sched\r\n");
    vTaskStartScheduler();
    log_puts("[DBG] ERR: sched returned\r\n");
    for (;;) { }
    return 0;
}
