# Отладка NUCLEO-H743ZI2

Кратко: как подключать отладчик к плате NUCLEO-H743ZI2 (встроенный ST-Link), смотреть код по шагам и ставить breakpoint'ы.

**Полный сценарий одной сессии** (запуск lwip под GDB + поиск причины сбоя + практика GDB): **[LWIP_DEBUG_SESSION_RU.md](LWIP_DEBUG_SESSION_RU.md)**.

---

## Порядок запуска тестирования (lwip на NUCLEO-H743ZI2)

| № | Где | Действие |
|---|-----|----------|
| 1 | Терминал 1 | Запустить `st-util` и дождаться `Listening at *:4242...` |
| 2 | Терминал 2 | `cd` в каталог проекта, выполнить `make lwip` |
| 3 | Терминал 2 | Запустить `./scripts/gdb_lwip.sh` |
| 4 | GDB | Ввести `c` (continue) — остановка на `main` или в fault-обработчике |
| 5 | Дальше | Ставить breakpoint'ы, шагать (`n`/`s`), смотреть переменные; при изменении кода — `load`, `monitor reset halt`, `c` |

Опционально: UART-лог (minicom/screen на `/dev/ttyACM0` или `/dev/ttyACM1`, 115200 8N1) — чтобы видеть вывод `log_puts` после входа в main.

---

## 1. Что нужно

- **Плата** NUCLEO-H743ZI2, подключённая по USB (ST-Link).
- **stlink-tools** — `st-util` (GDB-сервер), `st-flash` (прошивка):
  ```bash
  sudo apt install stlink-tools   # Linux
  ```
  Или сборка: [stlink-org/stlink](https://github.com/stlink-org/stlink).
- **arm-none-eabi-gdb** (часто идёт с `gcc-arm-none-eabi`).

Опционально в Cursor/VSCode: расширение **Cortex-Debug** (marus25.cortex-debug) — тогда можно запускать отладку по F5 без ручного `st-util`.

---

## 2. Общая схема

1. Собрать прошивку **с отладочными символами** (в Makefile уже есть `-g` для lwip/step1; для бутлоадера — `make -f bootloader/Makefile debug`).
2. В **первом терминале** запустить GDB-сервер: `st-util` (слушает порт **4242**).
3. Во **втором терминале** запустить GDB с нужным `.elf`, подключиться к st-util, при необходимости загрузить образ и поставить breakpoint'ы.

---

## 3. Отладка приложения lwip

### Пошагово (с нуля)

**Шаг 1.** Откройте два терминала. В **терминале 1** запустите GDB-сервер и не закрывайте его:

```bash
st-util
```

Должно появиться `Listening at *:4242...`

**Шаг 2.** В **терминале 2** соберите lwip и запустите GDB:

```bash
cd /path/to/stm32_secure_boot
make lwip
./scripts/gdb_lwip.sh
```

Скрипт сам: подключится к st-util, загрузит образ (`load`), сделает reset и halt, поставит breakpoint на `main` и остановится на приглашении `(gdb)`.

**Шаг 3.** В GDB нажмите **`c`** (или `continue`). Выполнение дойдёт до `main` и остановится. Дальше можно ставить другие breakpoint'ы, шагать (`n`/`s`), смотреть переменные.

**GDB перезапускать не нужно** — один раз подключились и работаете в той же сессии. Если пересобрали проект (`make lwip`), в том же GDB выполните снова `load` и при необходимости `monitor reset halt`, затем `c`.

---

### Если остановились в UsageFault_Handler (пошагово, без перезапуска GDB)

Вы уже в GDB, выполнение в `UsageFault_Handler`. Делайте по порядку **в том же сеансе GDB**:

| Шаг | Команда в GDB | Зачем |
|-----|----------------|-------|
| 1 | `p/x *(uint32_t*)0xE000ED28` | Причина сбоя (CFSR). Биты 16–31 = UFSR (0x0100=неверная инструкция, 0x0400=INVSTATE, 0x0800=невыровненный доступ, 0x1000=деление на ноль). |
| 2 | `x/xw $sp+24` | Адрес сбойной инструкции (PC из стека). Запомните число (например 0x08001234). |
| 3 | `x/i 0x08001234` | Подставьте свой адрес из шага 2. Покажет саму инструкцию и, если есть символы, файл:строка. |
| 4 | `monitor reset halt` | Сбросить плату и остановить в начале. |
| 5 | `c` | Запустить. Либо дойдёт до breakpoint на `main`, либо снова попадёте в UsageFault_Handler. |

Если снова попали в UsageFault_Handler — повторите шаги 1–3 (уже будет обновлённый `g_usage_fault_cfsr` после пересборки). Чтобы сузить место сбоя, перед шагом 5 поставьте breakpoint раньше по коду, например:

- `break Reset_Handler` — затем `c`, потом по шагам `n` (next);
- или `break main` и `c` — если до main доходит, значит сбой уже после входа в main.

**Пересобрали код?** В том же GDB: `load`, затем `monitor reset halt`, затем `c`. Перезапускать GDB не нужно.

---

### Сборка (справка)

```bash
make lwip
```

Сборка уже с `-g`, символы в `build/lwip/lwip.elf`.

### Терминал 1 — GDB-сервер

```bash
st-util
```

Оставить запущенным (должно быть `Listening at *:4242`).

### Терминал 2 — GDB

```bash
cd /path/to/stm32_secure_boot
./scripts/gdb_lwip.sh
```

Либо вручную:

```bash
arm-none-eabi-gdb build/lwip/lwip.elf
```

В GDB:

```
(gdb) target extended-remote localhost:4242
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

Дальше выполнение остановится на `main`. Можно ставить точки останова, например:

- `break main` — вход в программу
- `break StartDefaultTask` — вход в задачу FreeRTOS
- `break log_puts` — каждый вывод в UART
- `break ethernetif_init` — инициализация Ethernet

Команды: `continue` (c), `next` (n), `step` (s), `print переменная`, `backtrace` (bt).

---

## 4. Отладка step1 / step2

Аналогично: собрать (`make step1` или `make step2`), в одном терминале `st-util`, в другом:

```bash
arm-none-eabi-gdb build/step1/step1.elf
# или build/step2/step2.elf
(gdb) target extended-remote localhost:4242
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

---

## 5. Отладка бутлоадера

Подробно: [DEBUG_RU.md](DEBUG_RU.md).

Кратко: сборка с отладкой `make -f bootloader/Makefile debug`, затем `st-util` и:

```bash
./scripts/gdb_bootloader.sh
```

или вручную с `build/bootloader/bootloader.elf`.

---

## 6. Отладка из Cursor / VSCode

### Вариант A: встроенный C/C++ debugger (GDB + st-util)

1. Запустить **st-util** в отдельном терминале.
2. В Run and Debug выбрать конфигурацию **Debug lwip (GDB + st-util)** (если есть в `.vscode/launch.json`).
3. F5 — подключение к st-util, загрузка elf, останов на `main`.

Путь к GDB в launch.json: `miDebuggerPath` — например `arm-none-eabi-gdb` или полный путь, если не в PATH.

### Вариант B: Cortex-Debug

Установить расширение **Cortex-Debug**. Оно может само запускать st-util/openocd и подключать GDB — конфигурация в launch.json с `"servertype": "stutil"` и путём к elf.

---

## 7. Частые проблемы

| Проблема | Решение |
|----------|---------|
| Connection refused на 4242 | Запустить `st-util` в отдельном терминале и не закрывать его. |
| No symbol table / не видно имён функций | Собрать с `-g` (для lwip — `make lwip` уже с `-g`). |
| После load код «не тот» | Прошить актуальный образ (`make flash-lwip` и т.п.), затем в GDB снова `load` и `monitor reset halt`. |
| Плата не видна (st-util не находит) | Проверить USB-кабель и порт; `lsusb` (должен быть ST-Link); при необходимости переподключить плату. |

---

## 8. Полезные точки останова (lwip)

- **Reset_Handler** (в startup) — самый первый код после сброса.
- **main** — после инициализации в startup.
- **log_init** / **log_puts** — вывод в UART.
- **StartDefaultTask** — старт задачи, где вызываются tcpip_init, Netif_Config.
- **ethernetif_init** / **low_level_init** — инициализация Ethernet.
- **HAL_ETH_MspInit** — настройка пинов и прерываний ETH.

Если после reset нет UART и LED1 — ставить breakpoint в **Reset_Handler** и смотреть, доходит ли выполнение до включения LED1 и до `main`.

---

## 9. Остановился в UsageFault_Handler

Если при подключении GDB или после `continue` выполнение оказывается в **UsageFault_Handler** — произошёл Usage Fault (неверная инструкция, невыровненный доступ, деление на ноль и т.п.).

**В GDB сделайте:**

1. **Причина сбоя (CFSR/UFSR):**
   ```
   (gdb) p/x g_usage_fault_cfsr
   ```
   Биты 16–31 — UFSR. Типичные значения (ARM Cortex-M7):
   - `0x0100` — UNDEFINSTR (неверная инструкция)
   - `0x0200` — INVPC (неверный PC при исключении)
   - `0x0400` — INVSTATE (неверное состояние, например переход в Thumb с нечётным адресом)
   - `0x0800` — UNALIGNED (невыровненный доступ)
   - `0x1000` — DIVBYZERO (деление на ноль)

2. **Адрес сбойной инструкции** (PC в стеке при входе в handler):
   ```
   (gdb) x/xw $sp+24
   (gdb) x/i <полученный_адрес>
   ```
   По адресу можно найти строку в коде или смотреть дизассемблер.

3. **Сброс и повтор:**
   ```
   (gdb) monitor reset halt
   (gdb) break main
   (gdb) continue
   ```
   Если снова падает в UsageFault_Handler — ставьте breakpoint в **Reset_Handler**, затем **ExitRun0Mode**, **SystemInit**, **main** и пошагово (`next`), чтобы сузить место сбоя.
