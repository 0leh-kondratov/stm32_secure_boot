# Lista plików Makefile (bootloader i aplikacja)

Aby Cursor/Assembly mógł zbudować projekt, w pliku Makefile muszą zostać określone ścieżki do modułów z repozytorium **STM32CubeH7**. Korzeń kostki: `CUBE_ROOT ?= ../STM32CubeH7`.

---

## Core Drivers (HAL)

Ścieżka podstawowa: `$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src/`

| Moduł | Plik | Miejsce docelowe |
|--------|------|------------|
| Ethernet | `stm32h7xx_hal_eth.c` | LwIP, ETH DMA |
| I2C (LCD 1602) | `stm32h7xx_hal_i2c.c` | Wyświetlanie przez I2C |
| ECDSA (podpis) | `stm32h7xx_hal_pka.c` | **Uwaga:** w standardowej warstwie HAL dla STM32H743 nie ma sterownika PKA (PKA jest dostępna na L5/U5 itp.). W przypadku H743 podpis jest weryfikowany za pomocą mbedTLS w bootloaderze. Jeśli używasz płyty z PKA, dodaj ten plik. |
| SHA-256 | `stm32h7xx_hal_hash.c` | Integralność obrazu (program ładujący i/lub aplikacja) |

Dodatkowo dla HAL (już używane w app/lwip/Makefile):

- `stm32h7xx_hal.c`, `stm32h7xx_hal_cortex.c`, `stm32h7xx_hal_dma.c`, `stm32h7xx_hal_eth.c`, `stm32h7xx_hal_exti.c`, `stm32h7xx_hal_flash.c`, `stm32h7xx_hal_gpio.c`, `stm32h7xx_hal_pwr.c`, `stm32h7xx_hal_rcc.c`, `stm32h7xx_hal_tim.c`, `stm32h7xx_hal_uart.c`
- Legacy/Ex: `stm32h7xx_hal_rcc_ex.c`, `stm32h7xx_hal_pwr_ex.c`, `stm32h7xx_hal_uart_ex.c`, `stm32h7xx_hal_tim_ex.c`, `stm32h7xx_hal_flash_ex.c`
- Dla ETH: `stm32h7xx_hal_eth_ex.c` (w razie potrzeby)

---

## Middleware

### LwIP

- **Uwzględnij katalogi:**
  `$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/src/include`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/system`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/system/arch`
- **Źródła:** api, core, core/ipv4, netif; `system/arch/sys_arch.c` (CMSIS-RTOS2); dla HTTPS - altcp, altcp_tls (patrz lwipopts.h).

### FreeRTOS

- **Włączyć coś:**
  `$(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source/include`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2`  
+ port: `FreeRTOS/Source/portable/GCC/ARM_CM4F` (lub ARM_CM7)
- **Źródła:** zadania.c, kolejka.c, list.c, timers.c, port.c, sterta_4.c (lub inna sterta), `CMSIS_RTOS_V2/cmsis_os2.c`.

### mbedTLS (serwer HTTPS)

- **Włączyć coś:**
  `$(CUBE_ROOT)/Middlewares/Third_Party/mbedTLS/include`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/mbedTLS/include/mbedtls`
- **Źródła:** podłącz niezbędne mbedtls_*.c zgodnie z konfiguracją (patrz mbedtls_config.h w Cube). Dla TLS przez LwIP - port w `lwip/apps/` (altcp_tls_mbedtls).

---

## Końcowe ścieżki (zmienne Makefile)

```makefile
CUBE_ROOT ?= $(abspath ../STM32CubeH7)
HAL_SRC   = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
# Ethernet
HAL_ETH   = $(HAL_SRC)/stm32h7xx_hal_eth.c
# I2C (LCD 1602)
HAL_I2C   = $(HAL_SRC)/stm32h7xx_hal_i2c.c
# SHA-256
HAL_HASH  = $(HAL_SRC)/stm32h7xx_hal_hash.c
# PKA (jeśli jest obecny na docelowym chipie)
# HAL_PKA   = $(HAL_SRC)/stm32h7xx_hal_pka.c

LWIP_DIR    = $(CUBE_ROOT)/Middlewares/Third_Party/LwIP
FREERTOS_DIR = $(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS
MBEDTLS_DIR  = $(CUBE_ROOT)/Middlewares/Third_Party/mbedTLS
```

**app/lwip/Makefile** zawiera już HAL (eth, uart, tim, dma, ...), BSP Nucleo, LwIP i FreeRTOS. Aby uzyskać kompletną aplikację, dodaj do tego samego (lub osobnego) pliku Makefile: `stm32h7xx_hal_i2c.c`, `stm32h7xx_hal_hash.c`, sterownik LCD 1602, `lcd_monitor_task`, w razie potrzeby obiekty serwerów mbedTLS i HTTPS.
