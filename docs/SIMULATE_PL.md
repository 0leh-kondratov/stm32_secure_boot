# Emulacja bootloadera w Renode

Skrypt **simulate.resc** uruchamia obraz bootloadera w emulatorze [Renode](https://renode.io/) na platformie STM32H743 (Cortex-M7, flash, RAM, USART3 itp.). Wygodne sprawdzenie logiki bez płytki.

---

## 1. Zainstaluj Renode

- **Linux (Debian/Ubuntu):**  
  `sudo apt install renode`  
lub pobierz z [renode.io](https://renode.io/).
- **macOS/Windows:** instalator ze strony Renode.

Sprawdź: `renode --version`.

---

## 2. Montaż i uruchomienie

Z katalogu głównego projektu:

```bash
make -f bootloader/Makefile all
make signed-app
renode simulate.resc
```

Renode pobierze platformę ze swojego katalogu (`platforms/cpus/stm32h743.repl`), załaduje **build/bootloader/bootloader.elf** i **build/app/signed_app.bin** pod adresem 0x08010000. Bez podpisanej aplikacji w emulacji program ładujący wyświetli komunikat „FAIL: bad magic” i zatrzyma się.

---

## 3. Czego się spodziewać

- Po `start` procesor rozpoczyna wykonywanie z Reset_Handler, a następnie `main()`.
- Bootloader włącza diodę LED1, wywołuje `log_init()` i zapisuje do USART3. W oknie **usart3** powinny pojawić się linie takie jak `[boot] Bezpieczny bootloader`, `[boot] UART 115200 OK` itp.
- Weryfikacja podpisu w emulacji odczytuje pamięć pod adresem obrazu aplikacji (0x08010000). Jeśli jest pusty lub zawiera śmieci, sprawdzenie może zakończyć się niepowodzeniem (odcinek akceptuje dowolny obraz, ale odczyt poza granicami pamięci flash może zachowywać się inaczej). Do „czystego” sprawdzenia kłód wystarczy.

---

## 4. Ograniczenia emulacji

- Model STM32H743 w Renode nie jest kompletny: może brakować niektórych urządzeń peryferyjnych lub zachowywać się inaczej niż na sprzęcie.
- Taktowanie i czasy różnią się od rzeczywistej płyty - opóźnienia i UART mogą zachowywać się inaczej.
- Nie można zaimplementować HASH (sprzętowego SHA) i bloków kryptograficznych; gdy włączone jest prawdziwe ECDSA (bez kodu pośredniczącego), emulacja może nie dojść do punktu weryfikacji podpisu.

Emulacja nadaje się do debugowania przepływu sterowania (wejście do głównego, logi, przejście do aplikacji po pomyślnej weryfikacji) bez opłat.

---

##5a. Sprawdzanie emulacji na demo (demo.resc)

Aby przetestować Renode na **prostym działającym obrazie** bez bootloadera i podpisu, użyj **demo.resc** - tego samego samodzielnego demo (3 LED + UART) co na płycie:

```bash
make demo
renode demo.resc
```

W oknie **usart3** powinny pojawić się następujące linie:
- `Demo: LEDs + log (NUCLEO-H743ZI2)`
- `LED1=ON LED2=OFF LED3=OFF` i dalej w kółko.

Serwer GDB dla wersji demonstracyjnej nasłuchuje na porcie **3334** (aby nie zakłócać pliku Simulation.resc na 3333). Połączenie: `docelowy pilot: 3334`. W Kursorze możesz wybrać konfigurację **Debug demo (Renode)** po uruchomieniu `renode demo.resc`.

**GDB dla wersji demonstracyjnej (w skrócie):**
,,bicie
arm-none-eabi-gdb build/demo/demo.elf
(gdb) docelowy pilot:3334
(gdb) przerwać main
(gdb)kontynuuj
```

**Jak zobaczyć stan diody LED w emulacji:**
1. **W oknie usart3** - demo wyświetla już linie `LED1=ON LED2=OFF LED3=OFF` itd. (funkcja `log_led_state` w app/demo/main.c).
2. **W monitorze Renode** (konsola, na której uruchomiony jest renode) można odczytać rejestr GPIO ODR (Output Data Register) - pokazuje on aktualny poziom na wyjściach:
- `(maszyna-0) sysbus ReadDoubleWord 0x58020414` - **GPIOB ODR**: bit 0 = LED1 (PB0), bit 14 = LED3 (PB14); wartość 1 = wyjście wysokie (dioda świeci).
- `(maszyna-0) sysbus ReadDoubleWord 0x58021014` - **GPIOE ODR**: bit 1 = LED2 (PE1); 1 = świeci.
Przykład: jeśli pierwsze polecenie zwróciło `0x4001`, to ustawione są bity 0 i 14 → Świecą się diody LED1 i LED3.

---

## 5. Dodatkowe

- W **simulate.resc** przykład **AddHook** na wejściu do `main` jest zakomentowany - można go odkomentować w celu debugowania wyników podczas wchodzenia do main.
- Ścieżka do ELF: `@build/bootloader.elf` - w stosunku do katalogu, z którego uruchamiany jest Renode (uruchamiany z katalogu głównego projektu).

**GDB:** serwer GDB jest włączony w skrypcie na porcie **3333**. Najpierw uruchom `renode Simulator.resc`, następnie podłącz debuger: `arm-none-eabi-gdb build/bootloader.elf`, w GDB: `target Remote :3333`, `break main`, `continue`. Więcej szczegółów znajdziesz w sekcji „9. Debugowanie w Renode (GDB)” w [DEBUG_PL.md](DEBUG_PL.md). W Cursor/VSCode możesz wybrać konfigurację **Debuguj bootloader (Renode)** i naciśnij klawisz F5 (Renode musi już być uruchomiony).

---

## 6. „NIEPOWODZENIE: zła magia” na planszy

Jeśli taki log pochodzi z **prawdziwej płyty**, to w pamięci flash pod adresem 0x08010000 nie ma obrazu z nagłówkiem (magicznym `BOOT`). Musisz sflashować **połączony** obraz (program ładujący + aplikacja), a nie tylko program ładujący:

```bash
./scripts/flash.sh
```

Montuje bootloader, podpisuje aplikację i zapisuje jeden plik do flashowania: pierwsze 64 KB to bootloader, następnie od 0x08010000 to nagłówek i aplikacja. Następnie podczas Resetu weryfikacja podpisu powinna przejść pomyślnie (z USE_ECDSA_STUB - zawsze) i wykonanie zostanie przeniesione do aplikacji.
