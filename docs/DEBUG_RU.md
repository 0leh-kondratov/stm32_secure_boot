# Интерактивная отладка бутлоадера на STM32

Пошаговая инструкция: сборка с отладочными символами, запуск GDB-сервера (st-util), подключение отладчика и точки останова.

---

## 1. Что нужно

- Плата **NUCLEO-H743ZI2** по USB (ST-Link).
- **stlink-tools** (в т.ч. `st-util` и `st-flash`):  
  `sudo apt install stlink-tools` (Linux) или соберите из [stlink-org/stlink](https://github.com/stlink-org/stlink).
- **arm-none-eabi-gdb**: входит в пакет `gcc-arm-none-eabi` или отдельно.

Опционально для отладки из Cursor/VSCode: расширение **Cortex-Debug** (marus25.cortex-debug).

---

## 2. Сборка с отладочными символами

Из корня проекта:

```bash
make -f bootloader/Makefile debug
```

Собирается `build/bootloader/bootloader.elf` с флагами `-g -O0` (символы и без оптимизации). Файл `build/bootloader/bootloader.bin` тоже обновлён — его можно прошить перед отладкой.

**Если вы уже собирали бутлоадер без debug**, пересоберите с нуля с отладочными символами:

```bash
make -f bootloader/Makefile debug-clean
```

(это делает `clean` и затем `debug`). Или вручную: `make -f bootloader/Makefile clean && make -f bootloader/Makefile debug`.

---

## 3. Прошить образ (если ещё не прошит)

```bash
st-flash write build/bootloader/bootloader.bin 0x08000000
```

Либо полный сценарий с приложением: `./scripts/flash.sh`.

---

## 4. Запуск GDB-сервера (st-util)

В **первом терминале** запустите сервер отладки:

```bash
st-util
```

Должно появиться что-то вроде:

```
st-util 1.8.0
...
Listening at *:4242...
```

Оставьте этот терминал открытым. Порт **4242** — по нему будет подключаться GDB.

---

## 5. Подключение GDB (интерактивно)

### Вариант A: скрипт из проекта

Во **втором терминале**:

```bash
cd /path/to/stm32_secure_boot
./scripts/gdb_bootloader.sh
```

Скрипт подключается к `localhost:4242`, загружает `build/bootloader.elf`, делает `reset halt` и ставит точку останова на `main`. Дальше в GDB:

- `continue` (или `c`) — выполнение до следующего breakpoint.
- `next` (`n`), `step` (`s`) — пошагово.
- `print переменная` — значение переменной.
- `break verify_signature` — ещё одна точка останова.
- `quit` — выход.

### Вариант B: GDB вручную

```bash
arm-none-eabi-gdb build/bootloader.elf
```

В GDB:

```
(gdb) target extended-remote localhost:4242
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

Дальше выполнение остановится на `main`; можно ставить breakpoint’ы (например на `verify_signature`), смотреть переменные, шагать по коду.

---

## 6. Отладка из Cursor / VSCode

### Предварительно

1. Запустите **st-util** в отдельном терминале (п. 4).
2. В проекте есть конфигурации отладки в `.vscode/launch.json`.

### Конфигурация «Debug bootloader (GDB + st-util)»

- Выберите в панели Run and Debug конфигурацию **Debug bootloader (GDB + st-util)**.
- Нажмите **F5** (или Run). Сначала выполнится задача **Build debug bootloader**, затем GDB подключится к st-util, загрузит elf, сделает reset halt и поставит breakpoint на `main`.
- Поставьте при необходимости дополнительные точки останова в коде (например в `bootloader/src/main.c` на `verify_signature` или в начале `main`).
- Запуск/остановка: F5, F10 (step over), F11 (step into), просмотр переменных в панели слева.

Если GDB не подключается — проверьте, что **st-util** запущен и висит на порту 4242.

### Конфигурация «Debug bootloader (Cortex-Debug)»

Работает при установленном расширении **Cortex-Debug**. Она сама запускает st-util (или openocd), подключает отладчик и переходит в `main`. Удобно, если не хотите вручную держать st-util в отдельном терминале.

---

## 7. Полезные точки останова

В `bootloader/src/main.c`:

- **main** — вход в бутлоадер (после LED1 и log_init).
- **verify_signature** — перед проверкой заголовка и подписи.
- **jump_to_application** — перед переходом в приложение.

В `common/uart_log.c`: **log_init**, **log_puts** — если отлаживаете вывод в UART.

---

## 8. Частые проблемы

- **«Connection refused» на 4242** — не запущен st-util. Запустите его в отдельном терминале.
- **«No symbol table» / нет имён функций** — соберите с отладкой: `make -f bootloader/Makefile debug`.
- **После load код не совпадает с исходником** — прошейте актуальный образ: `st-flash write build/bootloader/bootloader.bin 0x08000000`, затем снова `load` и `monitor reset halt` в GDB.

Подробнее по сборке и прошивке: [BUILD_RU.md](BUILD_RU.md).

---

## 9. Отладка в Renode (GDB)

В эмуляции Renode тоже можно подключать GDB: в **simulate.resc** включён GDB-сервер на порту **3333**.

**Шаги:**

1. Соберите бутлоадер с отладочными символами и подписанное приложение:
   ```bash
   make -f bootloader/Makefile debug-clean
   make signed-app
   ```

2. Запустите Renode (эмуляция и GDB-сервер стартуют):
   ```bash
   renode simulate.resc
   ```

3. Во **втором терминале** подключите GDB:
   ```bash
   arm-none-eabi-gdb build/bootloader.elf
   ```
   В GDB:
   ```
   (gdb) target remote :3333
   (gdb) break main
   (gdb) continue
   ```
   Если эмуляция уже идёт — нажмите **Ctrl+C** в GDB, чтобы остановить CPU, поставьте breakpoint, затем `continue`.

4. Дальше как обычно: `next`, `step`, `print переменная`, breakpoint на `verify_signature` и т.д.

**Отладка приложения:** после перехода бутлоадера в приложение можно загрузить символы приложения и ставить breakpoint’ы в нём:
   ```
   (gdb) add-symbol-file build/app/app.elf 0x08010080
   (gdb) break main
   (gdb) continue
   ```
   (адрес 0x08010080 — начало кода приложения после заголовка; при другом entry point подставьте свой.)
