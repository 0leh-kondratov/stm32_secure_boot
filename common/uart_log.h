/*
 * UART log for STM32H743 (USART3 on PD8/PD9, 115200 8N1).
 * Used by bootloader and app to print to ST-Link VCP on NUCLEO-144.
 */
#ifndef COMMON_UART_LOG_H
#define COMMON_UART_LOG_H

void log_init(void);
void log_puts(const char *s);

#endif /* COMMON_UART_LOG_H */
