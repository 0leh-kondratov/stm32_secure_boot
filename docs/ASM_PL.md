# Asembler w projekcie: opis i zastosowanie

Projekt wykorzystuje **GNU Assembler** (ARM) jako kod startowy. Składnia: **ujednolicony**, docelowy procesor - **Cortex-M7**, tryb **Kciuk**.

---

## 1. Jakie pliki i dlaczego

| Plik | Miejsce docelowe |
|------|------------|
| **app/startup_stm32h743xx.s** | Uruchamianie aplikacji (demo, lwip, step1, step2, itp.). Wektory przerwań, inicjalizacja pamięci RAM, wywołanie `main`. Opcjonalnie: dioda wczesnego zapłonu LED2 (PE1). |
| **bootloader/startup_stm32h743xx.s** | Uruchomienie bootloadera. Ta sama struktura, ale bez zbędnych peryferii: tylko stos, kasowanie .bss/.data, opóźnienie i dioda LED1 (PB0), potem `main`. |

Obydwa pliki określają:
- tablica wektorów (pierwsze słowo to początkowe SP, drugie to licznik programu);
- punkt wejścia `Reset_Handler`;
- kody pośredniczące dla procedur obsługi wyjątków (NMI, HardFault, SysTick, SVC, PendSV itp.).

Linker umieszcza sekcję `.isr_vector` na początku FLASH (pod adresem ze skryptu, zwykle `0x08000000`), dzięki czemu procesor będzie ładował SP i PC od pierwszych dwóch słów po resecie.

---

## 2. Dyrektywy i składnia

Na początku każdego „.s”:

```asm
.syntax unified /* ujednolicona składnia ARM (wygodniejsza dla Thumb) */
.cpu cortex-m7
.fpu softvfp /* miękka emulacja FPU, jeśli to konieczne */
.thumb /* Tylko instrukcje kciukowe */
```

- **.global** — symbol eksportu dla linkera (na przykład `Reset_Handler`, `g_pfnVectors`).
- **.section .isr_vector,"a",%progbits** - sekcja tylko do odczytu, wyrównana, zawierająca tablicę wektorów.
- **.word** — jedna wartość 32-bitowa (adres lub numer procedury obsługi).
- **.thumb_func** — kolejna etykieta to funkcja w Thumb (poprawne przejście z `bx`/`bl`).
- **.weak** jest słabym symbolem: jeśli Twój własny `SysTick_Handler` jest gdzieś zdefiniowany, zostanie on zastąpiony.
- **.rept N** / **.endr** — powtórz blok N słów (rezerwa na wektory 16..255).

Symbole `_estack`, `_sbss`, `_ebss`, `_sdata`, `_edata`, `_sidata` są ustawiane przez **skrypt linkera** (na przykład `demo.ld`, `app.ld`).

---

## 3. Tabela wektorów przerwań (.isr_vector)

Lokalizacja pamięci musi odpowiadać oczekiwaniom jądra (często VTOR = 0x08000000). Kolejność słów ARM:

| Przesunięcie | Spis treści | Opis |
|----------|------------|----------|
| 0x00 | `_stos` | Wartość początkowa wskaźnika stosu (MSP). Zaczerpnięte z linkera: koniec pamięci RAM. |
| 0x04 | `Reset_Handler` | Resetuj - rdzeń przeskakuje tutaj po załadowaniu SP. |
| 0x08 | NMI_Handler | NMI. |
| 0x0C | HardFault_Handler | HardFault. |
| 0x10 | MemManage_Handler | MemManage. |
| 0x14 | BusFault_Handler | BusFault. |
| 0x18 | UsageFault_Handler | UsageFault. |
| 0x1C–0x28 | 0 lub zarezerwowane | Skryty. |
| 0x2C | Obsługa_SVC | SVC (używany przez FreeRTOS do wywoływania harmonogramu). |
| 0x30 | DebugMon_Handler | Debug monitor. |
| 0x34 | 0 | Reserved. |
| 0x38 | Obsługa PendSV | PendSV (przełączanie kontekstu we FreeRTOS). |
| 0x3C | SysTick_Handler | SysTick (znak systemu operacyjnego). |
| 0x40+ | 0 (240 słów) | Przerwania zewnętrzne (IRQ0–IRQ239); w razie potrzeby zastąpione prawdziwymi przewodnikami. |

W kodzie jest to określone przez sekwencję `.word`; puste miejsca - `.słowo 0`. Domyślne procedury obsługi to znaczniki prowadzące do nieskończonej pętli `b .` (lub słabe odcinki `b .` dla nadpisania SVC/PendSV/SysTick w C).

---

## 4. Reset_Handler: krok po kroku

Logika jest taka sama w aplikacji i bootloaderze; Jedyne różnice to „wczesna” dioda LED i opóźnienie.

### 4.1 Instalowanie stosu

```asm
ldr r0, =_estack
msr msp, r0
```

Wskaźnik stosu (MSP) jest określony na końcu adresu RAM („_estack” ze skryptu linkera). Bez tego każde wywołanie funkcji lub przerwanie ulegnie awarii.

### 4.2 Wczesna dioda LED (opcjonalna)

**app/startup:** Dioda LED2 (PE1) na NUCLEO jest włączona - wizualnie widać, że wykonanie osiągnęło start.

- RCC: `0x58024400` + 0xE0 → AHB4ENR; bit 4 = GPIOE.
- GPIOE: baza `0x58021000`; MODER (0x00) - wyjście na pin 1; BSRR (0x18) - ustawianie/resetowanie. Na NUCLEO diody LED są aktywne na niskim poziomie: wpisanie do bitu „reset” wyłącza BSRR, a do bitu „set” włącza go (kod wykorzystuje bity 1+16 do domyślnego wyłączenia lub odwrotnie, w zależności od obwodu).

**bootloader/uruchamianie:** to samo dla LED1 (PB0), plus krótkie opóźnienie cyklu, aby po resecie mruganie było zauważalne.

### 4.3 Czyszczenie pliku .bss

Niezainicjowane zmienne globalne i statyczne (zerowujące) znajdują się w sekcji `.bss`. Przed wywołaniem kodu C należy je zresetować:

```asm
ldr r0, =_sbss /* start .bss */
ldr r1, =_ebss /* koniec .bss */
mov r2, #0 /* wartość do zapisania (wymagana!) */
b 2f
1:  str r2, [r0]
    adds r0, r0, #4
2:  cmp r0, r1
    bcc 1b
```

Ważne: **r2 musi być jawnie ustawione na zero** (`mov r2, #0`). Bez tego śmieci dostają się do pliku .bss i zachowanie kodu C (w tym FreeRTOS) jest nieprzewidywalne.

### 4.4 Kopiowanie .data

Zainicjowane dane po załadowaniu znajdują się w pamięci FLASH (adres `_sidata`), a po wykonaniu muszą znajdować się w pamięci RAM pomiędzy `_sdata` a `_edata`:

```asm
ldr r0, =_sdata
ldr r1, =_edata
ldr r2, =_sidata
b 4f
3:  ldr r3, [r2]
    str r3, [r0]
    adds r0, r0, #4
    adds r2, r2, #4
4:  cmp r0, r1
    bcc 3b
```

Pętla kopiuje słowa z pamięci FLASH do pamięci RAM. Jeśli rozmiar .data wynosi zero, pętla nie jest wykonywana.

### 4.5 Przejdź do strony głównej

```asm
bl main
b .
```

Wywołaj `main` i pętlę w nieskończoność, jeśli `main` kiedykolwiek zwróci (standardowo nie powinno).

---

## 5. Obsługa wyjątków

Wszystkie domyślne procedury obsługi to kody pośredniczące:

```asm
.thumb_func
Default_Handler:
NMI_Handler:
HardFault_Handler:
...
B.    /* niekończąca się pętla */
```

Nazwy są takie same jak wektory. W C możesz zastąpić na przykład:

- **SysTick_Handler** - w wersji demonstracyjnej FreeRTOS funkcja `xPortSysTickHandler()` jest wywoływana z `main_freertos.c`.
- **SVC_Handler**, **PendSV_Handler** - są zastępowane przez makra w `FreeRTOSConfig.h` z `vPortSVCHandler` i `xPortPendSVHandler` (port FreeRTOS).

Słabe znaczniki (`.weak SVC_Handler` itp.) pozwalają linkerowi na podstawienie silnych znaków z C/innego asm.

---

## 6. Wbuduj plik Makefile

Przykład (demo):

```makefile
$(BUILD)/startup.o: $(TOP)/app/startup_stm32h743xx.s | $(BUILD)
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<
```

- Ten sam kompilator co dla C (`arm-none-eabi-gcc`), w trybie asemblera (`-c`, źródło `.s`).
- Flagi `-mcpu=cortex-m7 -mthumb` muszą odpowiadać docelowemu jądru i trybowi w `.cpu`/`.thumb`.
- Podczas łączenia do listy dodawany jest obiekt `startup.o`; Skrypt linkera umieszcza `.isr_vector` na początku obrazu.

Bootloader montuje swój `bootloader/startup_stm32h743xx.s` według podobnej zasady.

---

## 7. Jak dodać własne .s

1. Utwórz plik, np. `app/demo/extra.s`, z tymi samymi dyrektywami na początku (`.syntax unified`, `.cpu cortex-m7`, `.thumb`).
2. Zadeklaruj etykiety globalne: `.global My_Func`.
3. W pliku Makefile dodaj obiekt do zmiennej wraz z resztą `.o` i regułą:
   ```makefile
   $(BUILD)/extra.o: $(TOP)/app/demo/extra.s | $(BUILD)
   	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<
   ```
4. W C zadeklaruj prototyp w stylu `void My_Func(void);` i wywołaj go jak zwykłą funkcję.

Dopuszczalny jest także wbudowany asembler w C: `__asm ​​​​volatile("nop");` lub rozszerzony `asm` z operandami - bez edycji Makefile.

---

## 8. Komunikacja ze skryptem linkera

W `demo.ld` (i analogach) określono co następuje:

- **PAMIĘĆ** - FLASH i RAM, ich rozmiary.
- **_estack** - zwykle `POCHODZENIE(RAM) + DŁUGOŚĆ(RAM)`.
- **SECTIONS**:
- `.isr_vector` jest pierwszym w FLASH, `KEEP(*(.isr_vector))`.
- `.text` - kod.
- `.data` - zainicjalizowano dane w pamięci RAM za pomocą `LOADADDR(.data)` do kopiowania.
- `.bss` — niezainicjowane dane w pamięci RAM; etykiety `_sbss`, `_ebss`, `_sdata`, `_edata`, `_sidata` są wysyłane przez skrypt.

Bez poprawnych pętli `_sbss`/`_ebss`/`_sdata`/`_edata`/`_sidata` w Reset_Handler będzie działać niepoprawnie lub nadpisać niewłaściwą pamięć.

---

## 9. Krótko

| Element | Miejsce docelowe |
|--------|------------|
| **.isr_wektor** | Tabela wektorów: SP, Reset, NMI, Fault, SVC, PendSV, SysTick, IRQ. |
| **Reset_handler** | SP → inicjalizacja (opcja LED) → kasowanie .bss → kopiowanie .danych → `main`. |
| **.bss** | Pamiętaj o zresetowaniu; w pętli użyj rejestru jawnie ustawionego na 0. |
| **.dane** | Skopiuj z FLASH (_sidata) do RAM (_sdata.._edata). |
| **Osoby obsługi** | Odgałęzienia `b .`; SVC/PendSV/SysTick zostały ponownie zaimplementowane w C (FreeRTOS). |
| **Buduj** | `$(CC) -mcpu=kora-m7 -mthumb -c` dla każdego `.s`; obiekt uczestniczy w łączeniu. |

Dodatkowo: [Ogólny podręcznik użytkownika ARM Cortex-M7] (https://developer.arm.com/documentation/dui0646/latest), sekcja poświęcona wyjątkom i tabeli wektorów.
