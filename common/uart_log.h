/*
 * UART log for STM32H743 (USART3 on PD8/PD9, 115200 8N1).
 * Used by bootloader and app to print to ST-Link VCP on NUCLEO-144.
 */
#ifndef COMMON_UART_LOG_H
#define COMMON_UART_LOG_H

struct netif;

void log_init(void);
void log_puts(const char *s);
/* Log current IP address of netif to UART (e.g. when DHCP assigns or static set). */
void log_ip_address(struct netif *netif);
/* Non-blocking: returns received byte (0..255) or -1 if no data (for interactive shell). */
int log_getchar(void);

#endif /* COMMON_UART_LOG_H */
