# FreeRTOS: установка и запуск (STM32H743, NUCLEO-H743ZI2)

## 0. Интерактивный режим в демо

В демо (`make demo` → `build/demo/demo.bin`) включён **интерактивный шелл по UART**: подключите терминал к USART3 (PD8/PD9), 115200 8N1. После сброса появится приглашение `> `. Команды:

- **help** — список команд  
- **led1 on** / **led1 off** — ручное вкл/выкл LED1 (иначе авто-мигание)  
- **led2 on** / **led2 off** — то же для LED2  
- **led3 on** / **led3 off** — вкл/выкл LED3  
- **status** — текущее состояние LED  

Реализовано только на FreeRTOS (одна задача-шелл читает `log_getchar()` и парсит строки).

---

## 1. Откуда взять FreeRTOS

### Вариант A: из STM32CubeH7 (рекомендуется)

FreeRTOS уже входит в пакет **STM32CubeH7**. Исходники лежат в:

- `Middlewares/Third_Party/FreeRTOS/Source/` — ядро (tasks.c, queue.c, list.c, timers.c и т.д.)
- `Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/` — порт для Cortex-M4/M7 (ARM_CM4F подходит и для M7)
- `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/` — обёртка CMSIS-RTOS v2 (cmsis_os2.c и заголовки)
- `Drivers/CMSIS/RTOS2/Include/` — заголовки CMSIS-RTOS2

Готовый пример под **NUCLEO-H743ZI**:

- `Projects/NUCLEO-H743ZI/Applications/FreeRTOS/FreeRTOS_ThreadCreation/`

Его можно открыть в **STM32CubeIDE** и сразу собирать/запускать — это и есть «установленный и запущенный» FreeRTOS.

Если в вашей копии Cube нет папки `Middlewares/Third_Party/FreeRTOS`, скачайте полный пакет с [st.com](https://www.st.com/en/embedded-software/stm32cubeh7.html) (Get Software) или добавьте среду через **STM32CubeMX** (Middleware → FREERTOS → Enabled).

### Вариант B: с сайта FreeRTOS

- [FreeRTOS kernel](https://www.freertos.org/a00104.html) — скачать архив, распаковать.
- Порт для GCC/ARM Cortex-M: `FreeRTOS/Source/portable/GCC/ARM_CM4F/` (подходит и для M7).
- В своём проекте подключаете только нужные `.c` из `Source/` и порт, без CMSIS-RTOS — API будет нативный FreeRTOS (`xTaskCreate`, `vTaskStartScheduler` и т.д.).

---

## 2. Как «запустить» (минимальные шаги в коде)

Имеется в виду: после добавления исходников в сборку — что сделать, чтобы планировщик заработал.

### 2.1 Конфигурация

- Добавьте в проект файл **FreeRTOSConfig.h** (можно скопировать из примера NUCLEO-H743ZI и поправить):
  - `configTICK_RATE_HZ` — частота тика (обычно 1000 = 1 кГц, 1 тик = 1 мс).
  - `configTOTAL_HEAP_SIZE` — размер кучи FreeRTOS в байтах (для пары задач хватает 2–4 КБ).
  - `configCPU_CLOCK_HZ` — частота ядра (например `SystemCoreClock`).
  - При использовании CMSIS-RTOS2 оставьте включёнными флаги `configUSE_OS2_*` и `USE_FreeRTOS_HEAP_4` (или другой heap), как в примере Cube.

- В **линкере** убедитесь, что есть достаточно RAM под кучу (heap) и стеки задач; на NUCLEO-H743ZI2 с 128 КБ RAM — обычно без проблем.

### 2.2 Тактовый источник (tick)

FreeRTOS нужен периодический тик. В примерах STM32Cube это делает **SysTick** (обработчик из FreeRTOS/CMSIS-RTOS2, не HAL). Важно:

- Либо не инициализировать SysTick под HAL (тогда FreeRTOS сам вешает свой `SysTick_Handler`),
- Либо в `FreeRTOSConfig.h` оставить маппинг на стандартные имена (например `xPortSysTickHandler` в `stm32h7xx_it.c` вызывается из `SysTick_Handler` — в Cube это уже настроено в коде CMSIS-RTOS2).

То есть: после `HAL_Init()` и настройки часов тик для FreeRTOS уже идёт от SysTick; дополнительно «устанавливать» ничего не нужно, если используете пример из Cube.

### 2.3 Запуск планировщика

**Если используете CMSIS-RTOS v2** (как в STM32CubeH7):

```c
#include "cmsis_os2.h"
#include "FreeRTOS.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();   // ваша настройка тактирования

    osKernelInitialize();

    // Создание задач (потоков)
    osThreadNew(led_task, NULL, &led_attr);
    // ...

    osKernelStart();   // запуск планировщика — сюда не вернёмся
    for (;;) {}
}
```

**Если используете нативный FreeRTOS** (без CMSIS):

```c
#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    xTaskCreate(led_task, "LED", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);

    vTaskStartScheduler();   // сюда не вернёмся
    for (;;) {}
}
```

После `osKernelStart()` или `vTaskStartScheduler()` управление переходит к задачам; код после них не выполняется.

---

## 3. Сборка из командной строки (Makefile)

Чтобы собрать проект с FreeRTOS **вручную** (как ваш текущий `stm32_secure_boot` на Makefile):

1. Добавьте в `CFLAGS` пути к include:
   - `FreeRTOS/Source/include`
   - `FreeRTOS/Source/portable/GCC/ARM_CM4F`
   - при CMSIS-RTOS2: `FreeRTOS/Source/CMSIS_RTOS_V2`, `Drivers/CMSIS/RTOS2/Include`
2. Добавьте в сборку все нужные `.c`:
   - из `FreeRTOS/Source/`: `tasks.c`, `queue.c`, `list.c`, `timers.c`, `heap_4.c` (или другой heap)
   - из `portable/GCC/ARM_CM4F/`: `port.c`
   - при CMSIS-RTOS2: `cmsis_os2.c`
3. Добавьте в проект свой `FreeRTOSConfig.h` и определите `STM32H743xx`, `USE_HAL_DRIVER` и т.д., если они нужны для этого файла.

Пример путей относительно корня STM32CubeH7 (если подключаете Cube как внешнюю зависимость):

```makefile
CUBE = /path/to/STM32CubeH7
FREERTOS_SRC = $(CUBE)/Middlewares/Third_Party/FreeRTOS/Source
CMSIS_RTOS2  = $(FREERTOS_SRC)/CMSIS_RTOS_V2
PORT         = $(FREERTOS_SRC)/portable/GCC/ARM_CM4F

CFLAGS += -I$(FREERTOS_SRC)/include -I$(PORT) -I$(CMSIS_RTOS2) -I$(CUBE)/Drivers/CMSIS/RTOS2/Include
SRCS  += $(FREERTOS_SRC)/tasks.c $(FREERTOS_SRC)/queue.c $(FREERTOS_SRC)/list.c \
         $(FREERTOS_SRC)/timers.c $(FREERTOS_SRC)/heap_4.c \
         $(PORT)/port.c $(CMSIS_RTOS2)/cmsis_os2.c
```

---

## 4. Кратко

| Вопрос | Ответ |
|--------|--------|
| **Установить** | Взять исходники из STM32CubeH7 (`Middlewares/Third_Party/FreeRTOS`) или с freertos.org и добавить их в проект (IDE или Makefile). |
| **Запустить** | Настроить `FreeRTOSConfig.h`, дать FreeRTOS тик от SysTick, в `main()` вызвать `osKernelInitialize()` → создать задачи через `osThreadNew()` → `osKernelStart()` (или нативно `xTaskCreate` + `vTaskStartScheduler()`). |
| **Проверить без своего кода** | Открыть в STM32CubeIDE пример `Projects/NUCLEO-H743ZI/Applications/FreeRTOS/FreeRTOS_ThreadCreation`, собрать и прошить плату — два потока будут мигать LED. |

Документ UM1722 «Developing Applications on STM32Cube with RTOS» описывает интеграцию FreeRTOS в проектах ST в деталях.
