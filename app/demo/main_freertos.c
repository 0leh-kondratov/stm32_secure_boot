/*
* Demo z FreeRTOS: przełączanie LED1 → LED2 → LED3 → w kółko, 1 raz na 1 s.
* Żadnych podpowiedzi ani poleceń - tylko pętla i jedna linia w UART przy uruchomieniu.
* NUCLEO-H743ZI2 (MB1364): LED1=PB0, LED2=PE1, LED3=PB14 (demo sterowania).
* LD4 (ST-Link, nie kontrolowany przez MK):
* Czerwony - istnieje połączenie z komputerem, programator nie jest jeszcze aktywowany (brak oprogramowania/debugowania w IDE).
* Zielony - oprogramowanie układowe zostało pomyślnie ukończone lub sesja debugowania jest aktywna (oczekiwanie).
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

/* Wolny cykl: LED1 → LED2 → LED3 → LED1 ... (jedna świeci, pozostałe są wyłączone) */
#ifdef DEMO_RENODE_AUTO_CMD
/* W Renode SysTick często się nie zaznacza – vTaskDelay nie zwraca. Zajęty-czekaj ~1 s przy 64 MHz. */
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
/* Debugowanie: która dioda LED się świeci (L1/L2/L3) */
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

/* Po przedrostku „ledX” pomiń spacje i sprawdź, czy nie ma „włączonych”/„wyłączonych”. Zwraca 1=wł., 0=wył., -1=nie. */
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
if (*p == 'n' || *p == 'o') zwróć 1; /* on → „n”/„o”, gdy co drugi znak zostanie utracony */
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
/* W Renode: stan wyjścia co 5 sekund (cykl LED już się obraca w cyklu_zadania). */
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
/* W Renode \r/\n są często tracone (UART #487). Wysyłanie po przekroczeniu limitu czasu: pauza ~400 ms = wprowadzanie polecenia zakończone */
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
/* Wczesny UART i od razu LED - jasne, że doszliśmy do głównego, nawet jeśli dalej zamarzamy */
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
/* Renode: stos statyczny i TCB w .bss - ominięcie rozłączenia xTaskCreate (pvPortMalloc/sterta w emulatorze) */
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
