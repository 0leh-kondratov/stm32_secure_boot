# Список файлов для Makefile (Bootloader и Application)

Чтобы Cursor/сборка могли собрать проект, в Makefile должны быть прописаны пути к модулям из репозитория **STM32CubeH7**. Корень Cube: `CUBE_ROOT ?= ../STM32CubeH7`.

---

## Core Drivers (HAL)

Базовый путь: `$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src/`

| Модуль | Файл | Назначение |
|--------|------|------------|
| Ethernet | `stm32h7xx_hal_eth.c` | LwIP, ETH DMA |
| I2C (LCD 1602) | `stm32h7xx_hal_i2c.c` | Дисплей по I2C |
| ECDSA (подпись) | `stm32h7xx_hal_pka.c` | **Примечание:** в стандартном HAL для STM32H743 драйвера PKA нет (PKA есть на L5/U5 и др.). Для H743 подпись верифицируется через mbedTLS в bootloader. Если используете плату с PKA — добавьте этот файл. |
| SHA-256 | `stm32h7xx_hal_hash.c` | Целостность образа (bootloader и/или app) |

Дополнительно для HAL (уже используются в lwip/Makefile):

- `stm32h7xx_hal.c`, `stm32h7xx_hal_cortex.c`, `stm32h7xx_hal_dma.c`, `stm32h7xx_hal_eth.c`, `stm32h7xx_hal_exti.c`, `stm32h7xx_hal_flash.c`, `stm32h7xx_hal_gpio.c`, `stm32h7xx_hal_pwr.c`, `stm32h7xx_hal_rcc.c`, `stm32h7xx_hal_tim.c`, `stm32h7xx_hal_uart.c`
- Legacy/Ex: `stm32h7xx_hal_rcc_ex.c`, `stm32h7xx_hal_pwr_ex.c`, `stm32h7xx_hal_uart_ex.c`, `stm32h7xx_hal_tim_ex.c`, `stm32h7xx_hal_flash_ex.c`
- Для ETH: `stm32h7xx_hal_eth_ex.c` (если нужен)

---

## Middleware

### LwIP

- **Включить каталоги:**  
  `$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/src/include`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/system`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/system/arch`
- **Исходники:** api, core, core/ipv4, netif; `system/arch/sys_arch.c` (CMSIS-RTOS2); при HTTPS — altcp, altcp_tls (см. lwipopts.h).

### FreeRTOS

- **Включить:**  
  `$(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source/include`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2`  
  + порт: `FreeRTOS/Source/portable/GCC/ARM_CM4F` (или ARM_CM7)
- **Исходники:** tasks.c, queue.c, list.c, timers.c, port.c, heap_4.c (или другой heap), `CMSIS_RTOS_V2/cmsis_os2.c`.

### mbedTLS (HTTPS-сервер)

- **Включить:**  
  `$(CUBE_ROOT)/Middlewares/Third_Party/mbedTLS/include`  
  `$(CUBE_ROOT)/Middlewares/Third_Party/mbedTLS/include/mbedtls`
- **Исходники:** подключать нужные mbedtls_*.c по конфигурации (см. mbedtls_config.h в Cube). Для TLS через LwIP — порт в `lwip/apps/` (altcp_tls_mbedtls).

---

## Итоговые пути (переменные для Makefile)

```makefile
CUBE_ROOT ?= $(abspath ../STM32CubeH7)
HAL_SRC   = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
# Ethernet
HAL_ETH   = $(HAL_SRC)/stm32h7xx_hal_eth.c
# I2C (LCD 1602)
HAL_I2C   = $(HAL_SRC)/stm32h7xx_hal_i2c.c
# SHA-256
HAL_HASH  = $(HAL_SRC)/stm32h7xx_hal_hash.c
# PKA (если есть на целевом чипе)
# HAL_PKA   = $(HAL_SRC)/stm32h7xx_hal_pka.c

LWIP_DIR    = $(CUBE_ROOT)/Middlewares/Third_Party/LwIP
FREERTOS_DIR = $(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS
MBEDTLS_DIR  = $(CUBE_ROOT)/Middlewares/Third_Party/mbedTLS
```

В **lwip/Makefile** уже подключены HAL (eth, uart, tim, dma, …), BSP Nucleo, LwIP и FreeRTOS. Для полной Application добавьте в тот же (или отдельный) Makefile: `stm32h7xx_hal_i2c.c`, `stm32h7xx_hal_hash.c`, драйвер LCD 1602, задачу `lcd_monitor_task`, при необходимости mbedTLS и объекты HTTPS-сервера.
