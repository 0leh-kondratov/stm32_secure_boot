# Interaktywne debugowanie bootloadera na STM32

Instrukcje krok po kroku: budowanie z symbolami debugowania, uruchamianie serwera GDB (st-util), podłączanie debugera i punkty przerwania.

---

## 1. Czego potrzebujesz

- Płyta **NUCLEO-H743ZI2** przez USB (ST-Link).
- **narzędzia stlink** (w tym `st-util` i `st-flash`):
`sudo apt install stlink-tools` (Linux) lub skompiluj z [stlink-org/stlink] (https://github.com/stlink-org/stlink).
- **arm-none-eabi-gdb**: zawarte w pakiecie `gcc-arm-none-eabi` lub osobno.

Opcjonalnie do debugowania z Cursor/VSCode: rozszerzenie **Cortex-Debug** (marus25.cortex-debug).

---

## 2. Budowanie z symbolami debugowania

Z katalogu głównego projektu:

```bash
make -f bootloader/Makefile debug
```

`build/bootloader/bootloader.elf` jest zbudowany z flagami `-g -O0` (symbole i bez optymalizacji). Zaktualizowano także plik `build/bootloader/bootloader.bin` - można go sflashować przed debugowaniem.

**Jeśli już zbudowałeś bootloader bez debugowania**, przebuduj go od podstaw, używając symboli debugowania:

```bash
make -f bootloader/Makefile debug-clean
```

(odbywa się to poprzez „wyczyszczenie”, a następnie „debugowanie”). Lub ręcznie: `make -f bootloader/Makefile clean && make -f bootloader/Makefile debug`.

---

## 3. Flashuj obraz (jeśli jeszcze nie sflashowano)

```bash
st-flash write build/bootloader/bootloader.bin 0x08000000
```

Lub pełny skrypt z aplikacją: `./scripts/flash.sh`.

---

## 4. Uruchamianie serwera GDB (st-util)

W **pierwszym terminalu** uruchom serwer debugowania:

```bash
st-util
```

Powinno pojawić się coś takiego:

```
st-util 1.8.0
...
Listening at *:4242...
```

Zostaw ten terminal otwarty. Port **4242** - GDB się z nim połączy.

---

## 5. Podłączenie GDB (interaktywne)

### Opcja A: skrypt z projektu

W **drugim terminalu**:

```bash
cd /path/to/stm32_secure_boot
./scripts/gdb_bootloader.sh
```

Skrypt łączy się z `localhost:4242`, ładuje `build/bootloader.elf`, wykonuje `reset stop` i ustawia punkt przerwania na `main`. Dalej w GDB:

- `kontynuuj` (lub `c`) - wykonanie do następnego punktu przerwania.
- `dalej` (`n`), `krok` (`s`) - krok po kroku.
- `drukuj zmienną` — wartość zmiennej.
- `przerwa weryfikująca_podpis` to kolejny punkt przerwania.
- `quit` - wyjście.

### Opcja B: GDB ręcznie

```bash
arm-none-eabi-gdb build/bootloader.elf
```

W GDB:

```
(gdb) target extended-remote localhost:4242
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

Dalsze wykonywanie zatrzyma się na `main`; możesz ustawić punkty przerwania (na przykład na `verify_signature`), sprawdzić zmienne, przejść przez kod.

---

## 6. Debugowanie za pomocą kursora/VSCode

### Wstępne

1. Uruchom **st-util** w oddzielnym terminalu (krok 4).
2. Projekt posiada konfiguracje debugowania w `.vscode/launch.json`.

### Konfiguracja „Debuguj bootloader (GDB + st-util)”

- Wybierz konfigurację **Debuguj bootloader (GDB + st-util)** w panelu Uruchom i debuguj.
- Naciśnij **F5** (lub Uruchom). Najpierw zostanie wykonane zadanie **Buduj debugowanie bootloadera**, następnie GDB połączy się ze st-util, załaduje elf, zresetuje zatrzymanie i ustawi punkt przerwania na `main`.
- Jeśli to konieczne, ustaw dodatkowe punkty przerwania w kodzie (na przykład w `bootloader/src/main.c` w `verify_signature` lub na początku `main`.
- Start/stop: F5, F10 (przejście), F11 (wejście), przeglądanie zmiennych w lewym panelu.

Jeśli GDB nie łączy się, sprawdź, czy **st-util** działa i zawiesza się na porcie 4242.

### Konfiguracja „Debugowanie programu ładującego (Cortex-Debug)”

Działa, gdy zainstalowane jest rozszerzenie **Cortex-Debug**. Uruchamia st-util (lub openocd), łączy debuger i przechodzi do `main`. Wygodne, jeśli nie chcesz ręcznie przechowywać st-util w oddzielnym terminalu.

---

## 7. Przydatne punkty przerwania

W `bootloader/src/main.c`:

- **main** — wejście do bootloadera (po diodzie LED1 i log_init).
- **verify_signature** - przed sprawdzeniem nagłówka i podpisu.
- **przeskocz_do_aplikacji** - przed przejściem do aplikacji.

W `common/uart_log.c`: **log_init**, **log_puts** - jeśli debugujesz wyjście w UART.

---

## 8. Typowe problemy

- **„Odmowa połączenia” na 4242** - st-util nie działa. Uruchom go w osobnym terminalu.
- **"Brak tabeli symboli" / brak nazw funkcji** - kompiluj z debugowaniem: `make -f bootloader/debugowanie pliku Makefile`.
- **Po załadowaniu kod nie jest zgodny ze źródłem** - flashuj bieżący obraz: `st-flash zapisz build/bootloader/bootloader.bin 0x08000000`, następnie `load` ponownie i `monitor reset halt` w GDB.

Więcej szczegółów na temat montażu i oprogramowania sprzętowego: [BUILD_PL.md](BUILD_PL.md).

---

## 9. Debugowanie w Renode (GDB)

W emulacji Renode można także podłączyć GDB: w **simulate.resc** serwer GDB jest włączony na porcie **3333**.

**Kroki:**

1. Zbuduj bootloader z symbolami debugowania i podpisaną aplikacją:
   ```bash
   make -f bootloader/Makefile debug-clean
   make signed-app
   ```

2. Uruchom Renode (uruchomienie emulacji i serwera GDB):
   ```bash
   renode simulate.resc
   ```

3. W **drugim terminalu** podłącz GDB:
   ```bash
   arm-none-eabi-gdb build/bootloader.elf
   ```
W GDB:
   ```
   (gdb) target remote :3333
   (gdb) break main
   (gdb) continue
   ```
Jeśli emulacja już trwa, naciśnij **Ctrl+C** w GDB, aby zatrzymać procesor, ustaw punkt przerwania, a następnie „kontynuuj”.

4. Następnie jak zwykle: „następny”, „krok”, „drukuj zmienną”, punkt przerwania w „weryfikacji_podpisu” itp.

**Debugowanie aplikacji:** po przeniesieniu bootloadera do aplikacji możesz załadować symbole aplikacji i ustawić w niej breakpointy:
   ```
   (gdb) add-symbol-file build/app/app.elf 0x08010080
   (gdb) break main
   (gdb) continue
   ```
(adres 0x08010080 to początek kodu aplikacji po nagłówku; jeśli istnieje inny punkt wejścia, zastąp go własnym.)
