/**
 * @file uart_log.h
 * @brief UART logging for NUCLEO-H743ZI2 — USART3 (PD8 TX, PD9 RX), 115200 8N1.
 *
 * Robust HAL-based implementation. Call log_init() after SystemClock_Config()
 * so that the UART clock (D2PCLK1 / APB1) is correct for 115200 baud at 400 MHz.
 */
#ifndef COMMON_UART_LOG_H
#define COMMON_UART_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

struct netif;

/** Initialize USART3 for logging. Enable GPIOD and USART3 clocks, configure PD8/PD9, 115200 8N1. */
void log_init(void);

/** Blocking print of a null-terminated string. Uses HAL_UART_Transmit. */
void log_puts(const char *str);

/** Non-blocking: returns one received byte (0..255) or -1 if no data. */
int log_getchar(void);

/** Optional: log current IP of netif (e.g. LwIP). Weak stub if not overridden. */
void log_ip_address(struct netif *netif);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UART_LOG_H */
