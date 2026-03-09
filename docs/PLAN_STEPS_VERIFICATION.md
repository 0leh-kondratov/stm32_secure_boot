# План пошаговой реализации и тестирования (Steps 1–5 + LwIP/HTTP/DHCP/TLS)

План: Hardware Baseline → FreeRTOS → LwIP+MPU → Secure Boot (SHA-256) → Netconn (port 7) → опционально HTTP Server + DHCP + TLS.

---

## Сводка: демо-образы и команды

| Step | Цель | Сборка | Прошивка | Проверка |
|------|------|--------|----------|----------|
| **1** | I2C Scanner + LCD | `make step1` или `make test-stage1` | `make flash-step1` / `make flash-stage1` | UART: "Device at 0x27"; LCD: "Ready" |
| **2** | FreeRTOS + DisplayTask | `make step2` или `make test-stage2` | `make flash-step2` / `make flash-stage2` | LCD: счётчик раз в 1 с; SysTick стабилен |
| **3** | LwIP + Ethernet + MPU | `make lwip` (или step3) | `make flash-lwip` | `ping <IP>` с Ubuntu |
| **4** | Bootloader + SHA-256 | `make bootloader` + app | `./scripts/flash.sh` или вручную | UART: лог SHA-256, сравнение с эталоном |
| **5** | Netconn server port 7 + LCD echo | step5 (новый target) | flash-step5 | `telnet <IP> 7` → текст на LCD |
| **6** | HTTP + DHCP + TLS (опционально) | step6 / lwip-http | flash-step6 | HTTPS в браузере, DHCP IP |

Плата: **NUCLEO-H743ZI2** (MB1364). BOOT0=0. UART: USART3, 115200 8N1.

---

## Step 1: Hardware Baseline (I2C + LCD)

**Цель:** I2C1 (PB8/PB9), сканер, LCD 1602 (PCF8574 @ 0x27), без RTOS.

**Что есть в проекте:**
- **app/step1** — I2C + LCD baseline.
- **test/stage1_i2c_scanner** — I2C scanner, вывод в UART.

**Рекомендация:** Выбрать один образ для Step 1 (например step1 с LCD "Ready" + UART), чтобы не дублировать.

| Действие | Команда |
|----------|---------|
| Сборка | `make step1` **или** `make test-stage1` |
| Прошивка | `make flash-step1` **или** `make flash-stage1` |
| Проверка | 1) UART: строка вида "Device Found at 0x27" (или список адресов). 2) LCD: "Ready" (если step1 с LCD_Print). |

**Критерий успеха:** В UART виден адрес 0x27; на LCD — "Ready" (для step1).

---

## Step 2: OS Heartbeat (FreeRTOS + DisplayTask)

**Цель:** FreeRTOS, одна задача DisplayTask, счётчик на LCD раз в 1 с; HAL_Delay vs vTaskDelay согласованы.

**Что есть в проекте:**
- **app/step2** — FreeRTOS + DisplayTask, счётчик на LCD.
- **test/stage2_freertos_lcd** — FreeRTOS + LCD, "Wallet Init", счётчик.

| Действие | Команда |
|----------|---------|
| Сборка | `make step2` **или** `make test-stage2` |
| Прошивка | `make flash-step2` **или** `make flash-stage2` |
| Проверка | LCD: счётчик увеличивается каждую секунду; UART при наличии — стабильный вывод. SysTick не плавает (нет двойного инита SysTick). |

**Критерий успеха:** Счётчик на LCD раз в 1 с; система не зависает.

---

## Step 3: Network Physical Layer (LwIP + MPU)

**Цель:** Ethernet MAC, MPU (дескрипторы в SRAM3 0x30040000, Non-Cacheable), ping.

**Что есть в проекте:**
- **app/lwip** — LwIP + FreeRTOS, TCP echo (или минимальный стек); линкер `lwip_nucleo_h743zi.ld` (проверить секции под DMA/MPU).

**Задачи:** Убедиться, что в линкере есть регион под DMA descriptors (SRAM3); в коде — инициализация MPU для этой области (Non-Cacheable). При необходимости взять за основу Cube **LwIP_HTTP_Server_Netconn_RTOS** (ethernetif, MPU_Config).

| Действие | Команда |
|----------|---------|
| Сборка | `make lwip` |
| Прошивка | `make flash-lwip` |
| Проверка | Плата в одной подсети с ПК. `ping <IP>` с Ubuntu — ответ. IP — статический (задать в lwipopts.h/конфиге) или DHCP (см. Step 6). |

**Критерий успеха:** Успешный ping с хоста до платы.

---

## Step 4: Secure Bootloader (SHA-256 Integrity)

**Цель:** Проверка целостности образа приложения через HAL HASH (SHA-256); лог в UART, сравнение с эталоном.

**Что есть в проекте:**
- **bootloader/** — Secure Boot (ECDSA), при необходимости добавить/включить Verify_Integrity() на HAL HASH.
- Отдельный образ только под проверку SHA-256 можно вынести в step4 (bootloader + stub app) для изолированной проверки.

| Действие | Команда |
|----------|---------|
| Сборка | `make bootloader` и приложение (например `make app` + подпись или тестовый образ). |
| Прошивка | `./scripts/flash.sh` или вручную: bootloader @ 0x08000000, (signed) app @ 0x08010000. |
| Проверка | UART: лог вычисленного SHA-256 области приложения; сравнение с захардкоженным значением; при совпадении — переход в приложение (LED/логи). |

**Критерий успеха:** В логе бутлоадера — посчитанный SHA-256 и результат сравнения; при успехе — запуск приложения.

---

## Step 5: Interactive TCP (Netconn, port 7, echo на LCD)

**Цель:** Задача Netconn: слушать порт 7, принимать строку, выводить её на LCD (echo на экран).

**Реализация:** Новая цель в проекте (например **app/step5** или расширение **app/lwip**): LwIP Netconn server на порту 7, FreeRTOS task, при приёме данных — запись строки в буфер и вывод на LCD через существующий LCD API.

| Действие | Команда |
|----------|---------|
| Сборка | `make step5` (после добавления target) |
| Прошивка | `make flash-step5` |
| Проверка | `telnet <IP> 7`, ввод строки — та же строка появляется на LCD. |

**Критерий успеха:** telnet подключается, введённый текст отображается на LCD.

---

## Step 6 (опционально): LwIP HTTP Server + DHCP + TLS

**Вопрос:** можно ли добавить LwIP_HTTP_Server_Netconn_RTOS + DHCP + TLS?

**Да, по шагам:**

1. **HTTP + Netconn RTOS** — взять за основу пример из Cube:  
   `STM32CubeH7/Projects/NUCLEO-H743ZI/Applications/LwIP/LwIP_HTTP_Server_Netconn_RTOS`, перенести в проект (или скопировать ethernetif, http, netconn task) и собрать как отдельный образ (например **step6** или **lwip-http**).
2. **DHCP** — в lwipopts.h: `LWIP_DHCP 1`; в коде инициализации netif вызвать `dhcp_start()`. Проверка: плата получает IP от роутера; в логе/UART или на LCD можно вывести полученный IP.
3. **TLS (HTTPS)** — использовать LwIP **altcp_tls** (mbedTLS под капотом): поднять HTTPS-сервер на 443, сертификат/ключ на устройстве. Требует больше RAM/Flash и интеграции mbedTLS (из Cube или отдельно).

**Порядок внедрения:** сначала HTTP + DHCP (образ step6), проверка в браузере и по DHCP; затем добавить mbedTLS и altcp_tls для HTTPS.

| Действие | Команда (после реализации) |
|----------|----------------------------|
| Сборка | `make step6` или `make lwip-http` |
| Прошивка | `make flash-step6` |
| Проверка | DHCP: плата получает IP. Браузер: http://<IP> — страница. HTTPS: после интеграции TLS — https://<IP>. |

---

## Рекомендуемый порядок тестирования

1. **Step 1** — прошить step1 (или stage1), убедиться в UART + LCD.
2. **Step 2** — прошить step2 (или stage2), убедиться в счётчике раз в 1 с.
3. **Step 3** — прошить lwip, настроить сеть, проверить ping.
4. **Step 4** — прошить bootloader + app, проверить лог SHA-256 и переход в приложение.
5. **Step 5** — реализовать Netconn port 7 + LCD echo, прошить, проверить telnet и LCD.
6. **Step 6** — при необходимости: HTTP Server + DHCP, затем TLS.

Каждый шаг — отдельный демо-образ и чёткий критерий проверки (UART/LCD/ping/telnet/HTTP), чтобы изменения не ломали предыдущие этапы.

---

## Пути тестов

Оба stage собираются из каталога **test/**:
- **test-stage1** → `test/stage1_i2c_scanner`
- **test-stage2** → `test/stage2_freertos_lcd`
