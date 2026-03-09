# Финальная интеграция (Ethernet + LCD + FreeRTOS + Secure Boot)

Краткий чеклист по пунктам из запроса и что сделано.

---

## 1. Архитектура: Bootloader и Application

- **Bootloader:** `make bootloader` → `build/bootloader/bootloader.bin` (0x08000000). Проверка подписи (SHA-256 + ECDSA), при успехе — переход в приложение. См. `docs/ARCHITECTURE.md`.
- **Application:** LwIP + FreeRTOS + LCD + (опционально HTTPS). Сборка: `make lwip` → `build/lwip/lwip.bin`; образ подписывается и прошивается с 0x08010000.

---

## 2. Список файлов для Makefile

- **HAL:** `stm32h7xx_hal_eth.c`, `stm32h7xx_hal_i2c.c`, `stm32h7xx_hal_hash.c`.  
  `stm32h7xx_hal_pka.c` — только для чипов с PKA (на H743 нет); см. `docs/MAKEFILE_FILES_LIST.md`.
- **Middleware:** LwIP, FreeRTOS, mbedTLS (для HTTPS) — пути в `docs/MAKEFILE_FILES_LIST.md` и в `app/lwip/Makefile`.

---

## 3. I2C LCD 1602 в FreeRTOS

- **Драйвер:** `common/lcd_1602_i2c.c`, `common/lcd_1602_i2c.h` (адрес 0x27, PCF8574).
- **Задача:** `lcd_monitor_task` (имя в коде: `LcdMonitorTask`):
  - период 500 ms;
  - строка 1: текущий IP (из LwIP);
  - строка 2: `"Secure Boot OK"`;
  - доступ к I2C1 защищён **Mutex** (`I2C1_MutexHandle`).
- При старте на LCD выводится `"Booting Secure OS"` / `"..."`, затем после DHCP — IP.

---

## 4. Secure Boot (проверка перед прыжком)

- В `bootloader/src/main.c`: функция `verify_signature_and_ready_to_jump()` вызывает `verify_signature()` (SHA-256 по образу приложения + ECDSA). При успехе — `jump_to_application(hdr->entry_point)`, при ошибке — `signal_verification_failure()` (LED, останов).
- Комментарии в коде: на платах с PKA можно заменить на `HAL_PKA_VerifySignature(&hpka, &sig_params)`.
- Вывод на LCD из бутлоадера не реализован (нет I2C в бутлоадере); при необходимости можно добавить минимальный драйвер LCD в bootloader.

---

## 5. Сеть: HTTPS (LwIP + mbedTLS)

- В **lwipopts.h** включено: `LWIP_ALTCP 1`, `LWIP_ALTCP_TLS 1`. Для полноценного HTTPS нужно:
  - добавить mbedTLS в сборку (исходники и `mbedtls_config.h` из Cube);
  - включить порт LwIP для mbedTLS (`LWIP_ALTCP_TLS_MBEDTLS`, файлы из `app/lwip/apps/altcp_tls/`);
  - реализовать сервер на порту 443 на базе Netconn API поверх `altcp_tls` (по аналогии с примером LwIP_HTTP_Server_Netconn_RTOS в `Projects/NUCLEO-H743ZI/Applications/LwIP/`).
- Сейчас собран TCP echo (порт 7); HTTPS-сервер на 443 — следующий шаг при подключении mbedTLS.

---

## 6. Интеграция: память, MPU, целостность, heap

- **MPU:** В `main.c` вызывается `MPU_Config()`: регион для ETH DMA (0x30000000, 1 KB), регион для буферов LwIP (0x30004000, 16 KB). При использовании дескрипторов в **SRAM3 (0x30040000)** нужно добавить/скорректировать регион MPU под этот адрес (см. linker и ethernetif).
- **SHA-256 перед планировщиком:** В `main()` добавлен комментарий и заглушка вызова проверки целостности образа по адресу 0x08020000 (или 0x08010000). Реализация — через HAL HASH (`stm32h7xx_hal_hash.c`) в Application и сравнение с ожидаемым дайджестом; при необходимости раскомментировать и реализовать `firmware_integrity_check()`.
- **Heap FreeRTOS:** В `FreeRTOSConfig.h` задано **configTOTAL_HEAP_SIZE = 128 KB** для LwIP + mbedTLS + задач.
- **ethernetif:** Используется из примера H743ZI (или локальная копия), дескрипторы и буферы — в секциях, заданных в linker script (`lwip_nucleo_h743zi.ld`).

---

## Сборка

- Bootloader: `make bootloader`
- Application (LwIP + LCD + FreeRTOS): `make lwip`
- Подписанный образ приложения: `make signed-app` (после сборки app/lwip и подстановки правильного app.bin в скрипт подписи)

При необходимости перенести приложение на **0x08020000**: изменить ORIGIN во linker script приложения и адрес в скрипте подписи и в `memory_map.h`.
