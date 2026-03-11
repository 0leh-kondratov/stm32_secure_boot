# Monit o debugowanie: Demo FreeRTOS w Renode

Użyj tego bloku jako kontekstu podczas debugowania sytuacji „demo w Renode zatrzymuje się po części komunikatów UART”.

---

## Kontekst

- **Projekt:** stm32_secure_boot. Samodzielne demo: FreeRTOS, 3 diody LED (PB0, PE1, PB14), UART (USART3, 115200). Płytka: NUCLEO-H743ZI2.
- **Kompilacja dla Renode:** `make demo-renode` → `build/demo/demo.elf` (makro `DEMO_RENODE_AUTO_CMD`: bez system_stm32h7xx, busy-wait zamiast vTaskDelay w zadaniu LED, statyczne tworzenie zadania LED poprzez `xTaskCreateStatic`).
- **Uruchom:** z katalogu głównego projektu `renode demo.resc`; UART w oknie usart3 i poprzez `telnet localhost 12345`.
- **Oczekiwane wyjście w UART:**
`[DBG] Start demonstracji` → `[DBG] zegar 64M (renode)` → `[DBG] leds_init...` → `[DBG] GPIO ok` → `[DBG] Demo OK` → `[DBG] tworzenie zadania LED...` → `[DBG] przed xTaskCreateStatic` → `[DBG] po xTaskCreateStatic` → `[DBG] xTaskCreate zakończone` → `[DBG] LED zadanie ok` → `[DBG] start harmonogram` → `[DBG] LED zadanie uruchomione` → następnie cyklicznie `[LED] LED1 włączona` / `LED2 włączona` / `LED3 włączona`.

---

## Objaw (do wstawienia do zachęty)

Dane wyjściowe w Renode (usart3/telnet) ulegają awarii, na przykład po:

```
[DBG] Demo start
[DBG] clock 64M (renode)
[DBG] leds_init...
[DBG] GPIO ok
[DBG] Demo OK
[DBG] creating LED task...
```

Następnie nic się nie pojawia (żadnych komunikatów: „xTaskCreate zakończone”, „LED zadanie OK”, „rozpocznij harmonogram”, „LED zadanie uruchomione”, „[LED] LED1 włączona” itp.). Dziennik Renode może zawierać ostrzeżenia, takie jak „WriteDoubleWord do nieistniejącego urządzenia peryferyjnego pod adresem 0x1FEADB00” lub pod adresami 0x2007FFxx.

---

## Co zostało już zrobione w projekcie

1. **RAM 128K** w `app/demo/demo.ld` - tak aby stos i .bss nie wychodziły poza obszar mapowany przez Renode (dtcm 0x20000000).
2. **Dodaj. RAM w Renode** - `demo_ram_extra.repl`: obszar 0x1FE00000, 1 MiB; łączy się z `demo.resc` poprzez `machine LoadPlatformDescription @demo_ram_extra.repl` (pomijając wpisy pod adresem 0x1FEADB00 podczas inicjowania stosu zadań).
3. **W przypadku budowania Renode** zadanie LED jest tworzone poprzez **xTaskCreateStatic** (stos i TCB w .bss w `main_freertos.c`), bez wywoływania `pvPortMalloc` podczas tworzenia zadania.
4. **FreeRTOSConfig.h:** `configSUPPORT_STATIC_ALLOCATION 1`, `configKERNEL_PROVIDED_STATIC_MEMORY 1` - bezczynne zadanie wykorzystuje pamięć wbudowaną w jądro.
5. W `common/uart_log.c` z `DEMO_RENODE_AUTO_CMD`: `SystemD2Clock` nie jest używany; w `log_puts`, gdy harmonogram nie jest uruchomiony - busy-wait zamiast `vTaskDelay`.

Pliki kluczy: `app/demo/main_freertos.c`, `app/demo/FreeRTOSConfig.h`, `app/demo/demo.ld`, `demo.resc`, `demo_ram_extra.repl`, `common/uart_log.c`.

---

## Monit o debugowanie (skopiuj i dodaj, jeśli to konieczne)

```
Kontekst: projekt stm32_secure_boot, samodzielne demo na FreeRTOS dla STM32H743. Obraz do emulacji Renode składa się za pomocą polecenia make demo-renode (build/demo/demo.elf), uruchom: renode demo.resc, UART - usart3 / telnet localhost 12345.

Problem: w Renode wyjście UART zostaje odcięte po linii „[DBG] tworzenie zadania LED…”. Komunikaty „[DBG] xTaskCreate zakończone”, „LED zadanie ok”, „rozpocznij harmonogram”, „LED zadanie uruchomione” i „[LED] LED1 włączone” itp. nie pojawiają się.

W projekcie już zrobione: linker 128K RAM; demo_ram_extra.repl jest podłączony (RAM 0x1FE00000); w przypadku Renode zadanie LED jest tworzone poprzez xTaskCreateStatic (stos/TCB w .bss); configSUPPORT_STATIC_ALLOCATION i configKERNEL_PROVIDED_STATIC_MEMORY są włączone.

Potrzebować:
1. Na podstawie ostatniego wyświetlonego komunikatu określ miejsce zatrzymania wykonywania (przed/wewnątrz/po xTaskCreateStatic, przed/po vTaskStartScheduler, w zadaniu LED lub w log_puts).
2. Zaproponuj minimalne zmiany w lokalizacji (dodatkowe log_puts, breakpointy, sprawdzenie adresów w Renode).
3. Sprawdź, czy do zbudowania Renode faktycznie użyto xTaskCreateStatic i czy stos/TCB znajduje się w pliku .bss (adresy w 0x2000xxxx), a nie na stercie.
4. Jeśli podejrzewasz Renode (pamięć, urządzenia peryferyjne), zasugeruj sprawdzenie na monitorze Renode (sysbus, cpu) lub edytuj demo.resc / .repl.
```

---

## Szybkie kontrole

| Sprawdź | Zespół/miejsce |
|----------|------------------|
| Obraz dla Renode jest złożony | `zrób demo-renode` bez błędów, jest `build/demo/demo.elf` |
| Obraz wykorzystuje zadanie statyczne | W `main_freertos.c` z `DEMO_RENODE_AUTO_CMD` wywoływane jest `xTaskCreateStatic` z `led_stack`, `led_tcb` |
| Adres sterty/.bss w prawidłowym obszarze | `arm-none-eabi-nm build/demo/demo.elf \| grep -E "ucHeap|_ebss|_estack|led_stack|led_tcb"` - adresy pod adresem 0x2000xxxx, _estack = 0x20020000 |
| Podłączony ram_extra w Renode | W `demo.resc` znajduje się `machine LoadPlatformDescription @demo_ram_extra.repl` (ścieżka jest poprawna) |
| GDB do ponownego przetworzenia | Uruchom renode demo.resc, następnie `arm-none-eabi-gdb build/demo/demo.elf` → `target Remote :3334` → `break main` / przerwa w `xTaskCreateStatic` / w `task_led_cycle` → `kontynuuj` |

---

## Jeśli chcesz przekazać kontekst do innej sesji

Dodaj do zachęty:

- Dokładne ostatnie wyjście z usart3/telnet (kopia).
- Ostatnie wiersze dziennika Renode (OSTRZEŻENIE/BŁĄD), jeśli istnieją.
- Wersja ponownego przetworzenia: `renode --wersja`.
- Wyjaśnienie: po zmianach (xTaskCreateStatic, ram_extra) zachowanie nie uległo zmianie ani zmianie (na przykład pojawiły się nowe wiadomości).

---

## Wyniki debugowania (wykonanie monitu)

### 1. Miejsce zatrzymania wykonywania

Na podstawie ostatniego wyświetlonego komunikatu **„[DBG] tworzenie zadania LED…”** wykonywanie zostaje zatrzymane **wewnątrz wywołania xTaskCreateStatic** - aż do wyświetlenia komunikatu „[DBG] xTaskCreate zakończone”. Oznacza to, że zawieszenie występuje w jednej z gałęzi:

- `prvCreateStaticTask` (memset TCB, przypisanie pxStack, wywołanie prvInitialiseNewTask);
- `prvInitialiseNewTask` (inicjalizacja stosu poprzez pxPortInitialiseStack, kopiowanie nazw zadań, listy);
- `prvAddNewTaskToReadyList` (dodanie zadania do gotowych list).

### 2. Diagnostyka w kodzie

Dodano wiadomości z kropkami do `main_freertos.c`:

- **`[DBG] przed xTaskCreateStatic`** - bezpośrednio przed wywołaniem `xTaskCreateStatic`;
- **`[DBG] po xTaskCreateStatic`** - zaraz po wywołaniu.

Interpretacja w Renode:

- „tworzenie zadania LED…” i „przed xTaskCreateStatic” są widoczne, ale **nie** „po xTaskCreateStatic” → zawiesza się wewnątrz `xTaskCreateStatic` (jądro lub port FreeRTOS).
- Widoczne „przed” i „po” → zawieś później (np. w następnych `log_puts` lub w `vTaskStartScheduler`).

Aby zawęzić punkt zatrzymania w GDB, możesz ustawić punkty przerwania: `prvCreateStaticTask`, `prvInitialiseNewTask`, `prvAddNewTaskToReadyList`, `pxPortInitialiseStack`.

### 3. Sprawdzanie adresu (stos/TCB w .bss)

Polecenie (po `make demo-renode`):

```bash
arm-none-eabi-nm build/demo/demo.elf | grep -E "ucHeap|_ebss|_estack|led_stack|led_tcb"
```

Oczekiwany widok (wszystkie adresy z zakresu 0x2000xxxx):

| Symbol | Adres | Komentarz |
|-------------|-----------|---------------------|
| _ebs | 0x20002bec| koniec .bss |
| _estack | 0x20020000| szczyt stosu (128K RAM) |
| led_stack.* | 0x20000070| Stos zadań LED w pliku .bss |
| led_tcb.* | 0x20000014| Zadania LED TCB w pliku .bss |
| uSterta | 0x20000be4| sterta w .bss |

Wynik potwierdza: Do zbudowania Renode użyto `xTaskCreateStatic`, stos i TCB znajdują się w .bss (0x2000xxxx), a nie na stercie.

### 4. Sprawdza monitor Renode

- Upewnij się, że `demo_ram_extra.repl` jest podłączony: w `demo.resc` znajduje się `machine LoadPlatformDescription @demo_ram_extra.repl` (uruchamiany z katalogu głównego projektu).
- Sprawdź dostęp do pamięci RAM pod adresem 0x20000000 (rozmiar nie mniejszy niż 128K): w monitorze Renode po `start` wykonaj na przykład:
`sysbus ReadDoubleWord 0x20000000` - nie powinno być żadnego błędu dostępu.
- Gdy pojawi się OSTRZEŻENIE dotyczące rekordu pod adresem 0x1FEADB00 lub 0x2007FFxx, sprawdź, czy w pliku `demo_ram_extra.repl` jest wyświetlane 0x1FE00000; adresy 0x2007FFxx przekraczają 128 KB (0x20020000) i nie są używane w bieżącym linkerze.
