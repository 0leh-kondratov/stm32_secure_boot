# Step1 — LED1 + UART (ComIT, как UART_TwoBoards_ComIT)

На основе примера ST **UART_TwoBoards_ComIT**: включение LED1 и вывод сообщений в UART через **прерывания** (HAL_UART_Transmit_IT).

- **LED1:** PB0 (NUCLEO-H743ZI), активный уровень — низкий (для включения — PIN_RESET). Инициализация GPIO в `LED1_Init()`.
- **UART:** USART3 (PD8 TX, PD9 RX), 115200 8N1. Режим IT: `HAL_UART_Transmit_IT()`, флаг `UartTxReady` выставляется в `HAL_UART_TxCpltCallback()`. В `stm32h7xx_hal_msp.c` для USART3 включены прерывания (NVIC).
- **Поведение:** после старта включается LED1, по UART отправляются две строки (по очереди, с ожиданием завершения передачи по флагу), затем бесконечный цикл; LED1 остаётся включённым.

## Файлы

- `main.c` — MPU, Cache, часы, `LED1_Init()`, включение LED1, инициализация UART, два вызова `HAL_UART_Transmit_IT()` с ожиданием `UartTxReady`, `HAL_UART_TxCpltCallback` / `HAL_UART_ErrorCallback`.
- `stm32h7xx_hal_msp.c` — для USART3: PeriphCLK, GPIO, включение тактов, **NVIC** (SetPriority, EnableIRQ / DisableIRQ в DeInit).
- `stm32h7xx_it.c` — `USART3_IRQHandler()` → `HAL_UART_IRQHandler(&UartHandle)`.

## Сборка и прошивка

```bash
make step1
make flash-step1
```

Терминал 115200 8N1 на USART3 (PD8/PD9). После сброса: LED1 мигает раз в 500 мс, каждые 2 с в UART строка `Step 1: OK`.

**Индикаторы (NUCLEO-H743ZI2):** LED1 (PB0) — управляется приложением (мигание). LD4 (ST-Link): **красный** — связь с ПК есть, программатор не задействован; **зелёный** — прошивка/отладка выполнена или сессия в ожидании.

## Renode

Эмуляция тем же образом в Renode (платформа STM32H743, USART3, доп. RAM для FreeRTOS):

```bash
make step1-renode
renode step1.resc
```

Вывод UART — в окне usart3 и по `telnet localhost 12345`. GDB: `target remote :3334`.
