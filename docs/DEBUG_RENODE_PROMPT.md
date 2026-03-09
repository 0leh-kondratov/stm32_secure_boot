# Промпт для отладки: Demo FreeRTOS в Renode

Используй этот блок как контекст при отладке ситуации «демо в Renode останавливается после части сообщений UART».

---

## Контекст

- **Проект:** stm32_secure_boot. Standalone demo: FreeRTOS, 3 LED (PB0, PE1, PB14), UART (USART3, 115200). Плата: NUCLEO-H743ZI2.
- **Сборка для Renode:** `make demo-renode` → `build/demo/demo.elf` (макрос `DEMO_RENODE_AUTO_CMD`: без system_stm32h7xx, busy-wait вместо vTaskDelay в задаче LED, статическое создание LED-задачи через `xTaskCreateStatic`).
- **Запуск:** из корня проекта `renode demo.resc`; UART в окне usart3 и по `telnet localhost 12345`.
- **Ожидаемый вывод в UART:**  
  `[DBG] Demo start` → `[DBG] clock 64M (renode)` → `[DBG] leds_init...` → `[DBG] GPIO ok` → `[DBG] Demo OK` → `[DBG] creating LED task...` → `[DBG] before xTaskCreateStatic` → `[DBG] after xTaskCreateStatic` → `[DBG] xTaskCreate done` → `[DBG] LED task ok` → `[DBG] start sched` → `[DBG] LED task running` → далее циклически `[LED] LED1 on` / `LED2 on` / `LED3 on`.

---

## Симптом (для вставки в промпт)

Вывод в Renode (usart3 / telnet) обрывается, например после:

```
[DBG] Demo start
[DBG] clock 64M (renode)
[DBG] leds_init...
[DBG] GPIO ok
[DBG] Demo OK
[DBG] creating LED task...
```

Дальше ничего не появляется (нет `xTaskCreate done`, `LED task ok`, `start sched`, `LED task running`, `[LED] LED1 on` и т.д.). В логе Renode могут быть предупреждения вида `WriteDoubleWord to non existing peripheral at 0x1FEADB00` или по адресам 0x2007FFxx.

---

## Что уже сделано в проекте

1. **RAM 128K** в `app/demo/demo.ld` — чтобы стек и .bss не выходили за пределы области, которую Renode отображает (dtcm 0x20000000).
2. **Доп. RAM в Renode** — `demo_ram_extra.repl`: область 0x1FE00000, 1 MiB; подключается в `demo.resc` через `machine LoadPlatformDescription @demo_ram_extra.repl` (обход записей по 0x1FEADB00 при инициализации стека задачи).
3. **Для сборки Renode** LED-задача создаётся через **xTaskCreateStatic** (стек и TCB в .bss в `main_freertos.c`), без вызова `pvPortMalloc` при создании задачи.
4. **FreeRTOSConfig.h:** `configSUPPORT_STATIC_ALLOCATION 1`, `configKERNEL_PROVIDED_STATIC_MEMORY 1` — idle-задача использует встроенную в ядро память.
5. В `common/uart_log.c` при `DEMO_RENODE_AUTO_CMD`: не используется `SystemD2Clock`; в `log_puts` при не запущенном планировщике — busy-wait вместо `vTaskDelay`.

Ключевые файлы: `app/demo/main_freertos.c`, `app/demo/FreeRTOSConfig.h`, `app/demo/demo.ld`, `demo.resc`, `demo_ram_extra.repl`, `common/uart_log.c`.

---

## Промпт для отладки (скопировать и при необходимости дополнить)

```
Контекст: проект stm32_secure_boot, standalone demo на FreeRTOS для STM32H743. Образ для эмуляции Renode собирается командой make demo-renode (build/demo/demo.elf), запуск: renode demo.resc, UART — usart3 / telnet localhost 12345.

Проблема: в Renode вывод по UART обрывается после строки "[DBG] creating LED task...". Не появляются "[DBG] xTaskCreate done", "LED task ok", "start sched", "LED task running" и "[LED] LED1 on" и т.д.

В проекте уже сделано: линкер 128K RAM; подключён demo_ram_extra.repl (RAM 0x1FE00000); для Renode LED-задача создаётся через xTaskCreateStatic (стек/TCB в .bss); configSUPPORT_STATIC_ALLOCATION и configKERNEL_PROVIDED_STATIC_MEMORY включены.

Нужно:
1. По последнему выведённому сообщению определить, где выполнение останавливается (до/внутри/после xTaskCreateStatic, до/после vTaskStartScheduler, в задаче LED или в log_puts).
2. Предложить минимальные изменения для локализации (доп. log_puts, брейкпоинты, проверка адресов в Renode).
3. Проверить, что для сборки Renode действительно используется xTaskCreateStatic и что стек/TCB лежат в .bss (адреса в 0x2000xxxx), а не в куче.
4. Если есть подозрение на Renode (память, периферия) — предложить точечные проверки в мониторе Renode (sysbus, cpu) или правки demo.resc / .repl.
```

---

## Быстрые проверки

| Проверка | Команда / место |
|----------|------------------|
| Образ для Renode собран | `make demo-renode` без ошибок, есть `build/demo/demo.elf` |
| В образе используется статическая задача | В `main_freertos.c` при `DEMO_RENODE_AUTO_CMD` вызывается `xTaskCreateStatic` с `led_stack`, `led_tcb` |
| Адрес кучи / .bss в допустимой области | `arm-none-eabi-nm build/demo/demo.elf \| grep -E "ucHeap|_ebss|_estack|led_stack|led_tcb"` — адреса в 0x2000xxxx, _estack = 0x20020000 |
| Подключён ram_extra в Renode | В `demo.resc` есть `machine LoadPlatformDescription @demo_ram_extra.repl` (путь корректен) |
| GDB к Renode | Запустить renode demo.resc, затем `arm-none-eabi-gdb build/demo/demo.elf` → `target remote :3334` → `break main` / break в `xTaskCreateStatic` / в `task_led_cycle` → `continue` |

---

## Если нужно передать контекст другой сессии

Добавь к промпту:

- Точный последний вывод из usart3/telnet (скопировать).
- Последние строки лога Renode (WARNING/ERROR), если есть.
- Версию Renode: `renode --version`.
- Уточнение: после изменений (xTaskCreateStatic, ram_extra) поведение не изменилось или изменилось (например, появились новые сообщения).

---

## Результаты отладки (выполнение промпта)

### 1. Где останавливается выполнение

По последнему выведенному сообщению **"[DBG] creating LED task..."** выполнение останавливается **внутри вызова xTaskCreateStatic** — до вывода "[DBG] xTaskCreate done". То есть зависание происходит в одной из веток:

- `prvCreateStaticTask` (memset TCB, присвоение pxStack, вызов prvInitialiseNewTask);
- `prvInitialiseNewTask` (инициализация стека через pxPortInitialiseStack, копирование имени задачи, списки);
- `prvAddNewTaskToReadyList` (добавление задачи в готовые списки).

### 2. Диагностика в коде

В `main_freertos.c` добавлены точечные сообщения:

- **`[DBG] before xTaskCreateStatic`** — непосредственно перед вызовом `xTaskCreateStatic`;
- **`[DBG] after xTaskCreateStatic`** — сразу после вызова.

Интерпретация в Renode:

- Видны "creating LED task..." и "before xTaskCreateStatic", но **нет** "after xTaskCreateStatic" → зависание внутри `xTaskCreateStatic` (ядро FreeRTOS или порт).
- Видны "before" и "after" → зависание позже (например, в следующем `log_puts` или в `vTaskStartScheduler`).

Для сужения места остановки в GDB можно ставить брейкпоинты: `prvCreateStaticTask`, `prvInitialiseNewTask`, `prvAddNewTaskToReadyList`, `pxPortInitialiseStack`.

### 3. Проверка адресов (стек/TCB в .bss)

Команда (после `make demo-renode`):

```bash
arm-none-eabi-nm build/demo/demo.elf | grep -E "ucHeap|_ebss|_estack|led_stack|led_tcb"
```

Ожидаемый вид (все адреса в диапазоне 0x2000xxxx):

| Символ      | Адрес     | Комментарий        |
|-------------|-----------|---------------------|
| _ebss       | 0x20002bec| конец .bss          |
| _estack     | 0x20020000| верх стека (128K RAM) |
| led_stack.* | 0x20000070| стек LED-задачи в .bss |
| led_tcb.*   | 0x20000014| TCB LED-задачи в .bss |
| ucHeap      | 0x20000be4| куча в .bss         |

Вывод подтверждает: для сборки Renode используется `xTaskCreateStatic`, стек и TCB лежат в .bss (0x2000xxxx), не в куче.

### 4. Проверки в мониторе Renode

- Убедиться, что подключён `demo_ram_extra.repl`: в `demo.resc` есть `machine LoadPlatformDescription @demo_ram_extra.repl` (запуск из корня проекта).
- Проверить доступ к RAM по 0x20000000 (размер не меньше 128K): в мониторе Renode после `start` выполнить, например:  
  `sysbus ReadDoubleWord 0x20000000` — не должно быть ошибки доступа.
- При появлении WARNING о записи по 0x1FEADB00 или 0x2007FFxx — проверить, что 0x1FE00000 отображается через `demo_ram_extra.repl`; адреса 0x2007FFxx выходят за 128K (0x20020000) и в текущем линкере не используются.
