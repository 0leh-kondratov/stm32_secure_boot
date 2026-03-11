# Krok 1 - LED1 + UART (ComIT, jak UART_TwoBoards_ComIT)

Na przykładzie ST **UART_TwoBoards_ComIT**: włączenie diody LED1 i wysyłanie komunikatów do UART poprzez **przerwania** (HAL_UART_Transmit_IT).

- **LED1:** PB0 (NUCLEO-H743ZI), poziom aktywny - niski (do włączenia - PIN_RESET). Inicjowanie GPIO w `LED1_Init()`.
- **UART:** USART3 (PD8 TX, PD9 RX), 115200 8N1. Tryb IT: `HAL_UART_Transmit_IT()`, flaga `UartTxReady` jest ustawiona w `HAL_UART_TxCpltCallback()`. Przerwania (NVIC) są włączone w `stm32h7xx_hal_msp.c` dla USART3.
- **Zachowanie:** po starcie włącza się dioda LED1, wysyłane są dwie linie przez UART (kolejno oczekiwanie na zakończenie transmisji flagą), po czym pętla nieskończona; LED1 pozostaje włączona.

## Pliki

- `main.c` - MPU, Cache, zegar, `LED1_Init()`, włączenie LED1, inicjalizacja UART, dwa wywołania do `HAL_UART_Transmit_IT()` z oczekiwaniem `UartTxReady`, `HAL_UART_TxCpltCallback` / `HAL_UART_ErrorCallback`.
- `stm32h7xx_hal_msp.c` - dla USART3: PeriphCLK, GPIO, włączenie zegara, **NVIC** (SetPriority, EnableIRQ / DisableIRQ w DeInit).
- `stm32h7xx_it.c` — `USART3_IRQHandler()` → `HAL_UART_IRQHandler(&UartHandle)`.

## Montaż i oprogramowanie sprzętowe

```bash
make step1
make flash-step1
```

Terminal 115200 8N1 na USART3 (PD8/PD9). Po resecie: LED1 miga raz na 500 ms, co 2 s na linii UART „Krok 1: OK”.

**Wskaźniki (NUCLEO-H743ZI2):** LED1 (PB0) - sterowane przez aplikację (miga). LD4 (ST-Link): **czerwony** - istnieje połączenie z komputerem PC, programator nie jest używany; **zielony** — oprogramowanie układowe/debugowanie zakończone lub sesja w toku.

## Renode

Emulacja analogicznie w Renode (platforma STM32H743, USART3, dodatkowa pamięć RAM dla FreeRTOS):

```bash
make step1-renode
renode step1.resc
```

Wyjście UART - w oknie usart3 i poprzez `telnet localhost 12345`. GDB: `docelowy pilot: 3334`.
