# FreeRTOS: instalacja i uruchomienie (STM32H743, NUCLEO-H743ZI2)

## 0. Tryb interaktywny w wersji demonstracyjnej

Demo (`make demo` → `build/demo/demo.bin`) zawiera **interaktywną powłokę poprzez UART**: podłącz terminal do USART3 (PD8/PD9), 115200 8N1. Po zresetowaniu pojawi się znak zachęty `>`. Zespoły:

- **pomoc** — lista poleceń
- **led1 włączona** / **led1 wyłączona** — ręczne włączenie/wyłączenie diody LED1 (w przeciwnym razie automatyczne miganie)
- **led2 włączona** / **led2 wyłączona** — to samo dla LED2
- **led3 włączona** / **led3 wyłączona** — włączenie/wyłączenie LED3
- **status** — aktualny stan diody LED

Zaimplementowano tylko we FreeRTOS (jedno zadanie powłoki odczytuje `log_getchar()` i analizuje ciągi znaków).

---

## 1. Skąd zdobyć FreeRTOS

### Opcja A: z STM32CubeH7 (zalecane)

FreeRTOS jest już zawarty w pakiecie **STM32CubeH7**. Źródła znajdują się w:

- `Middlewares/Third_Party/FreeRTOS/Source/` - rdzeń (tasks.c, kolejka.c, list.c, timers.c itp.)
- `Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/` - port dla Cortex-M4/M7 (ARM_CM4F jest również odpowiedni dla M7)
- `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/` - Opakowanie CMSIS-RTOS v2 (cmsis_os2.c i nagłówki)
- `Drivers/CMSIS/RTOS2/Include/` - Nagłówki CMSIS-RTOS2

Gotowy przykład dla **NUCLEO-H743ZI**:

- `Projects/NUCLEO-H743ZI/Applications/FreeRTOS/FreeRTOS_ThreadCreation/`

Można go otworzyć w **STM32CubeIDE** i natychmiast skompilować/uruchomić – jest to FreeRTOS „zainstalowany i działający”.

Jeśli Twoja kopia Cube nie ma folderu `Middlewares/Third_Party/FreeRTOS`, pobierz pełny pakiet z [st.com](https://www.st.com/en/embedded-software/stm32cubeh7.html) (Pobierz oprogramowanie) lub dodaj środowisko poprzez **STM32CubeMX** (Middleware → FREERTOS → Włączone).

### Opcja B: ze strony internetowej FreeRTOS

- [Jądro FreeRTOS](https://www.freertos.org/a00104.html) - pobierz archiwum, rozpakuj.
- Port dla GCC/ARM Cortex-M: `FreeRTOS/Source/portable/GCC/ARM_CM4F/` (pasuje również do M7).
- W swoim projekcie podłącz tylko niezbędne `.c` z `Source/` i portu, bez CMSIS-RTOS - API będzie natywnym FreeRTOS (`xTaskCreate`, `vTaskStartScheduler`, itp.).

---

## 2. Jak „uruchomić” (minimalne kroki w kodzie)

Oznacza to: co po dodaniu źródeł do zestawu, co należy zrobić, aby harmonogram działał?

### 2.1 Konfiguracja

- Dodaj do projektu plik **FreeRTOSConfig.h** (można skopiować z przykładu NUCLEO-H743ZI i poprawić):
- `configTICK_RATE_HZ` - częstotliwość taktowania (zwykle 1000 = 1 kHz, 1 tick = 1 ms).
- `configTOTAL_HEAP_SIZE` — Rozmiar sterty FreeRTOS w bajtach (2–4 KB wystarczy na kilka zadań).
- `configCPU_CLOCK_HZ` - częstotliwość rdzenia (na przykład `SystemCoreClock`).
- Podczas korzystania z CMSIS-RTOS2 pozostaw włączone flagi `configUSE_OS2_*` i `USE_FreeRTOS_HEAP_4` (lub innej sterty), jak w przykładzie kostki.

- W **linkerze** upewnij się, że jest wystarczająca ilość pamięci RAM dla stosów sterty i zadań; na NUCLEO-H743ZI2 ze 128 KB RAM - zwykle nie ma problemu.

### 2.2 Źródło zegara (zaznacz)

FreeRTOS wymaga okresowego zaznaczenia. W przykładach STM32Cube odbywa się to za pomocą **SysTick** (program obsługi z FreeRTOS/CMSIS-RTOS2, a nie HAL). Ważny:

- Albo nie inicjuj SysTick pod HAL (wtedy sam FreeRTOS dodaje swój własny `SysTick_Handler`),
- Lub w `FreeRTOSConfig.h` zostaw mapowanie na standardowe nazwy (na przykład `xPortSysTickHandler` w `stm32h7xx_it.c` jest wywoływany z `SysTick_Handler` - w Cube jest to już skonfigurowane w kodzie CMSIS-RTOS2).

To znaczy: po `HAL_Init()` i ustawieniu zegara, znacznik FreeRTOS pochodzi już z SysTick; Nie musisz niczego dodatkowo „instalować”, jeśli skorzystasz z przykładu z Cube.

### 2.3 Uruchamianie harmonogramu

**Jeśli używasz CMSIS-RTOS v2** (jak w STM32CubeH7):

```c
#include "cmsis_os2.h"
#include "FreeRTOS.h"

int main(void)
{
    HAL_Init();
SystemClock_Config();   // ustawienie zegara

    osKernelInitialize();

// Utwórz zadania (wątki)
    osThreadNew(led_task, NULL, &led_attr);
    // ...

osKernelStart();   // uruchom harmonogram - nie będziemy tu wracać
    for (;;) {}
}
```

**Jeśli używasz natywnego FreeRTOS** (bez CMSIS):

```c
#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    xTaskCreate(led_task, "LED", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);

vTaskStartScheduler();   //nie będziemy tu wracać
    for (;;) {}
}
```

Po przekazaniu kontroli `osKernelStart()` lub `vTaskStartScheduler()` do zadań; kod po nich nie jest wykonywany.

---

## 3. Kompiluj z wiersza poleceń (Makefile)

Aby zbudować projekt za pomocą FreeRTOS **ręcznie** (jak twój obecny `stm32_secure_boot` na Makefile):

1. Dodaj ścieżki dołączania do `CFLAGS`:
   - `FreeRTOS/Source/include`
   - `FreeRTOS/Source/portable/GCC/ARM_CM4F`
- dla CMSIS-RTOS2: `FreeRTOS/Source/CMSIS_RTOS_V2`, `Drivers/CMSIS/RTOS2/Include`
2. Dodaj wszystkie niezbędne `.c` do zestawu:
- z `FreeRTOS/Source/`: `tasks.c`, `queue.c`, `list.c`, `timers.c`, `heap_4.c` (lub inny stos)
- z `portable/GCC/ARM_CM4F/`: `port.c`
- dla CMSIS-RTOS2: `cmsis_os2.c`
3. Dodaj plik `FreeRTOSConfig.h` do projektu i zdefiniuj `STM32H743xx`, `USE_HAL_DRIVER` itp., jeśli jest to potrzebne dla tego pliku.

Przykład ścieżek względem katalogu głównego STM32CubeH7 (jeśli podłączysz Cube jako zależność zewnętrzną):

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

## 4. Shell i FreeRTOS-Plus-CLI

W **jądrze FreeRTOS** nie ma wbudowanej powłoki - jedynie harmonogram, zadania, kolejki. W wersji demonstracyjnej projektu powłoka jest implementowana ręcznie: zadanie odczytuje UART („log_getchar()”), gromadzi ciąg znaków i analizuje polecenia w `run_cmd()`.

**FreeRTOS-Plus-CLI** to osobna biblioteka z rodziny FreeRTOS-Plus (ten sam dostawca). Daje:

- rejestracja poleceń (nazwa, ciąg znaków pomocy, wywołanie zwrotne);
- parametry analizy („FreeRTOS_CLIGetParameter()” itp.);
- jedno wywołanie dla całej linii wejściowej.

Wejście/wyjście nadal jest własne: UART, bufor linii i zadanie, które przekazuje gotowe linie do `FreeRTOS_CLIProcessInput()`.

| Sytuacja | Zalecenie |
|----------|--------------|
| Kilka poleceń (jak teraz: pomoc, led1/2/3, status) | Obecna minimalna powłoka jest wystarczająca, CLI nie jest wymagane. |
| Wiele poleceń, parametrów, ogólna pomoc | Dodanie **FreeRTOS-Plus-CLI** ma sens: łatwiej jest dodawać polecenia i analizować argumenty. |

Dokumentacja i źródła: [FreeRTOS-Plus-CLI](https://www.freertos.org/Documentation/03-Libraries/02-FreeRTOS-plus/03-FreeRTOS-plus-CLI/01-FreeRTOS-plus-CLI). Źródła można pobrać z repozytorium FreeRTOS/FreeRTOS (katalog FreeRTOS-Plus) lub z [freertos.org](https://www.freertos.org/a00104.html). Integracja: dodaj `FreeRTOS_CLI.c`, zaimplementuj wyjście (na przykład poprzez `log_puts()`), w zadaniu powłoki po zebraniu linii, wywołaj `FreeRTOS_CLIProcessInput()` zamiast `run_cmd()`.

---

## 5. Krótko

| Pytanie | Odpowiedź |
|--------|--------|
| **Zainstaluj** | Pobierz źródła z STM32CubeH7 („Middlewares/Third_Party/FreeRTOS”) lub z freertos.org i dodaj je do projektu (IDE lub Makefile). |
| **Uciekaj** | Skonfiguruj `FreeRTOSConfig.h`, zaznacz FreeRTOS w SysTick, wywołaj `osKernelInitialize()` w `main()` → utwórz zadania poprzez `osThreadNew()` → `osKernelStart()` (lub natywnie `xTaskCreate` + `vTaskStartScheduler()`). |
| **Sprawdź bez kodu** | Otwórz przykład `Projects/NUCLEO-H743ZI/Applications/FreeRTOS/FreeRTOS_ThreadCreation` w STM32CubeIDE, zmontuj i flashuj płytkę - dioda LED dwóch wątków będzie migać. |
| **Powłoka/CLI** | W jądrze nie ma powłoki. Obecna powłoka jest Twoja. Aby rozszerzyć obsługę na wiele poleceń, możesz dodać FreeRTOS-Plus-CLI (patrz sekcja 4). |

Dokument UM1722 „Tworzenie aplikacji na STM32Cube z RTOS” szczegółowo opisuje integrację FreeRTOS z projektami ST.
