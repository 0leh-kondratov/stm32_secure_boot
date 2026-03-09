# Архитектура проекта STM32 Secure Boot

Проект разделён на две части: **Bootloader (Secure Boot)** и **Application (Ethernet + LCD + FreeRTOS)**.

---

## 1. Bootloader (Secure Boot)

- **Расположение во Flash:** `0x08000000` — первые 64 KB.
- **Назначение:**
  - Инициализация HAL, тактов, UART (логирование).
  - Проверка подписи приложения (ECDSA P-256 над SHA-256 образа).
  - При успехе — переход в Application (`Jump_To_Application`).
  - При ошибке — индикация (LED) и останов.
- **Проверка подписи:** SHA-256 по региону приложения (HAL HASH или прямой доступ к HASH), верификация ECDSA (mbedTLS или заглушка `USE_ECDSA_STUB`). На платах с PKA возможна замена на `HAL_PKA_VerifySignature` (см. комментарии в `bootloader/src/main.c`).
- **Сборка:** `make bootloader` → `build/bootloader/bootloader.bin`. Прошивка в `0x08000000`.

---

## 2. Application (Ethernet + LCD + FreeRTOS)

- **Расположение во Flash:** образ с заголовком с `0x08010000` (или при необходимости с `0x08020000` — см. `memory_map.h` и скрипт подписи).
- **Назначение:**
  - Сеть: LwIP (Ethernet) + DHCP, при необходимости HTTPS (mbedTLS + LwIP altcp_tls).
  - Дисплей: I2C LCD 1602 (адрес 0x27), задача `lcd_monitor_task` обновляет IP и статус.
  - ОС: FreeRTOS + CMSIS-RTOS V2.
- **Перед стартом планировщика:** опциональная проверка целостности (SHA-256 по образу прошивки, например по региону `0x08020000` или текущему образу) через HAL HASH.
- **Сборка:** `make lwip` (или отдельный target Application с полным списком модулей) → образ для прошивки после заголовка (подписанный — `make signed-app`).

---

## 3. Разделение памяти (пример)

| Область        | Адрес        | Размер   | Описание                    |
|----------------|--------------|----------|-----------------------------|
| Bootloader     | 0x08000000   | 64 KB    | Secure Boot                  |
| App header+code| 0x08010000   | до 2MB   | Заголовок + приложение      |
| SRAM (DMA ETH) | 0x30040000   | по MPU   | Дескрипторы LwIP (SRAM3)    |
| Heap FreeRTOS  | —            | ≥ 128 KB | configTOTAL_HEAP_SIZE       |

---

## 4. Поток загрузки

1. Сброс → выполнение Bootloader.
2. Bootloader: проверка подписи образа приложения.
3. При успехе: `Jump_To_Application(entry_point)` (VTOR, MSP, переход на Reset_Handler приложения).
4. Application: HAL init, часы, опционально SHA-256 целостности, BSP, инициализация ядра FreeRTOS, создание задач (LwIP, LCD, HTTPS и т.д.), `osKernelStart()`.

---

## 5. Список файлов для Makefile

См. **docs/MAKEFILE_FILES_LIST.md** — пути к Core/HAL (Ethernet, I2C, HASH, PKA при наличии), LwIP, FreeRTOS, mbedTLS.
