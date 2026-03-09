/*
 * Minimal bootloader: only UART log test.
 * Use to verify that log output works (115200 8N1, USART2 PA2/PA3 or USART3 PD8/PD9).
 * Build: make minimal
 * Flash: st-flash write build/bootloader_minimal.bin 0x08000000
 */
#include "uart_log.h"

int main(void)
{
    log_init();
    log_puts("\r\n");
    log_puts("=== Bootloader log test ===\r\n");
    log_puts("If you see this, UART is OK.\r\n");
    log_puts("115200 8N1\r\n");
    log_puts("========================\r\n");

    for (;;) {
        __asm volatile("wfi");
    }
    return 0;
}
