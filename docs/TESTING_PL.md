# Używanie przykładów do testowania

Jak uruchomić istniejące przykłady i jak możesz pomóc w projekcie (raporty wyników, nowe przypadki testowe, skrypty).

**Struktura:** aplikacje (demo, lwip, krok1, krok2) - w `app/`; testuj oprogramowanie sprzętowe (skaner stage1 I2C, stage2 FreeRTOS LCD) - w `test/`. Kompilacja nadal pochodzi z katalogu głównego: „make demo”, „make step1”, „make test-stage1” itd.

---

## 1. Jakie są przykłady

| Przykład | Zbuduj zespół | Co flashować | Oczekiwanie po zresetowaniu |
|------------|----------------|------------|--------|
| **Oficjalny szablon ST** | `uczyń demo oficjalnym` | `ustaw wersję demonstracyjną Flash jako oficjalną` | Migają 3 diody LED (bez naszego bootloadera). Potrzebujemy klona STM32CubeH7 w pobliżu (`CUBE_ROOT`). |
| **Samodzielne demo projektu** | `zrób demo` | `stwórz demo flash` | 3 diody LED + log UART (115200 8N1, USART3). FreeRTOS, bez bootloadera. |
| **Minimalny program ładujący** | `make -f program ładujący/Makefile minimal-usart3` | `st-flash zapis kompilacji/bootloader_minimal_usart3.bin 0x08000000` | Dioda LED1 świeci, linia UART to `=== Test dziennika programu ładującego ===`. |
| **Pełne bezpieczne uruchamianie** | `./scripts/flash.sh` | ten sam skrypt miga sam | LED1 ~1 s, następnie dziennik bootloadera, następnie 3 diody LED i dziennik aplikacji. |

Płytka: NUCLEO-H743ZI2 (MB1364). BOOT0 = 0 (bootowanie z Flash).

---

## 2. Pierwszy w Renode (bez opłat)

Wygodnie jest sprawdzić obraz w emulatorze przed flashowaniem oprogramowania układowego.

| Obraz | Montaż | Uruchom Renode | okno usart3 |
|-------|------------|----------------|------------|
| **Demo (FreeRTOS)** | `zrób demo` | `renode demo.resc` | „Demo UART OK”, zachęta „>”, pomoc dotycząca poleceń, włączenie/wyłączenie diody LED3 |
| **Bootloader + aplikacja** | `utwórz podpisaną aplikację bootloadera` | `renode symulacja.resc` | Dziennik bootloadera, następnie aplikacja |

Z katalogu głównego projektu (gdzie znajdują się `demo.resc`, `simulate.resc`). GDB: w skryptach serwer jest podnoszony do 3334 (demo) lub 3333 (symulacja), podłącz „docelowy zdalny :3334” do odpowiedniego ELF.

### Debugowanie w kursorze/kodzie VS (GDB + Renode na hoście lokalnym)

Projekt zawiera plik `.vscode/launch.json` z konfiguracją połączenia z serwerem Renode GDB:

1. **Złóż obraz** (jeden z):
   - `stwórz demo` → dla „Debuguj demo (Renode)”
   - `utwórz podpisaną aplikację bootloadera` → dla „Debuguj bootloader+aplikację (symulacja ponownego przetworzenia)”

2. **Uruchom Renode** w terminalu z katalogu głównego projektu:
   - dla wersji demonstracyjnej: `renode demo.resc`
   - dla bootloadera: `renode Simulator.resc`

3. **W kursorze:** Uruchom → Rozpocznij debugowanie (F5), wybierz konfigurację (demo lub bootloader+aplikacja). Debuger połączy się z `localhost:3334` lub `localhost:3333` i załaduje symbole z odpowiedniego `.elf`.

Potrzebujesz rozszerzenia **C/C++** (ms-vscode.cpptools). Jeśli `arm-none-eabi-gdb` nie znajduje się w PATH, podaj pełną ścieżkę w `launch.json` w polu `miDebuggerPath`.

### Debugowanie sprzętowe (ST-Link, dane debugowania w kursorze)

Aby otrzymać dane debugowania (punkty przerwania, zmienne, stos wywołań) z prawdziwej tablicy:

1. **Wrzuć obraz** na tablicę (jednorazowo):  
   `zrób demo flash`.

2. **Uruchom serwer ST-Link GDB** w osobnym terminalu:
   ,,bicie
   st-util
   ```
   Serwer nasłuchuje na porcie **61234**. Płytkę należy podłączyć poprzez USB (ST-Link).

3. **W kursorze:** Uruchom → Rozpocznij debugowanie (F5), wybierz **Debuguj demo (ST-Link, sprzęt)**.

4. Debugger połączy się z płytką, załaduje symbole z `.elf` i zatrzyma się na wejściu do `main` (jeśli `stopAtEntry: true`). Dalej: ustaw punkty przerwania, spójrz na zmienne, wykonaj kod krok po kroku. Nadal spójrz na wyjście UART w minicom.

**Uruchamianie Renode:** z katalogu głównego projektu (`cd /data/projects/stm32_secure_boot`), w przeciwnym razie skrypty i ścieżki do `build/` nie zostaną znalezione.

Skrypty ustawione są na `logLevel 3` - w konsoli wyświetlane są tylko błędy; Ostrzeżenia RCC/peryferyjne (niezaimplementowane rejestry modelu STM32H743) są ukryte. Okno **usart3** powinno pokazać dziennik, nawet jeśli w konsoli pojawiły się ostrzeżenia.

---

## 3. Jak krok po kroku przeprowadzić testy

### 2.1 Kontrola środowiska (bez opłat)

Uruchom skrypt, który zbiera wszystkie cele (bez oprogramowania układowego):

```bash
cd /data/projects/stm32_secure_boot
./scripts/build_all.sh
```

Lub ręcznie:

```bash
cd /data/projects/stm32_secure_boot
make clean && make bootloader
make -f app/Makefile all
make signed-app
make demo
make -f bootloader/Makefile minimal-usart3
```

Wszystkie cele muszą być zmontowane bez błędów. Jeśli coś się zawiesi, wyślij wyjście `make` i wersję `arm-none-eabi-gcc --version`.

### 2.2 Z płytą: sprawdź procedurę

1. **Podłącz NUCLEO przez USB**, sprawdź:  
   `st-info --sonda`  
   Musi być jeden programator (ST-Link).

2. **Oficjalny szablon (podstawowy test planszy):**  
   `ustaw demo jako oficjalne && wykonaj oficjalne demo flash`  
   Reset → 3 diody LED powinny migać. Jeśli nie, sprawdź BOOT0 i zasilanie.

3. **Samodzielne demo (nasz kod, bez bootloadera):**  
   „zrób demo”.  
   `st-flash zapis build/demo/demo.bin 0x08000000`  
   Reset → 3 diody LED + logowanie minicom/putty 115200 8N1 (USART3/VCP).

4. **Minimalny bootloader (tylko log + LED1):**  
   `make -f bootloader/Makefile minimal-usart3`  
   `st-flash zapis kompilacji/bootloader_minimal_usart3.bin 0x08000000`  
   Reset → Świeci się dioda LED1, w UART: `=== Test logu bootloadera ===`.

5. **Scenariusz pełnego bezpiecznego rozruchu:**  
   `./scripts/flash.sh`  
   Reset → najpierw dioda LED1 i dziennik bootloadera, następnie migające 3 diody i dziennik aplikacji.

Po każdym kroku możesz spisać co widziałeś (LED/UART/błąd) i na jakiej płycie/firmwie.

---

## 4. Jak możesz pomóc

### 3.1 Raport z wyników testu

Uruchom skrypty z kroku 2 i wyślij krótki raport w następującym formacie:

- **Płyta:** NUCLEO-H743ZI2 (lub inna).
- **Środowisko:** OS, wersja `arm-none-eabi-gcc`, `st-flash`, Python.
- **Wyniki dla punktów 2.2:**  
  na przykład: „2 - OK, 3 - OK, 4 - dioda LED jest obecna, UART 0 bajtów, 5 - po zresetowaniu dioda LED i dziennik znikają.”

Pomoże Ci to zrozumieć, w jakich konfiguracjach co działa, i powielić problemy.

### 3.2 Dodatkowe przypadki testowe

Przydatne scenariusze, które można dodać lub opisać:

- Firmware **tylko aplikacje** do 0x08010000 (bez bootloadera) - zgodnie z oczekiwaniami, nie powinien zaczynać się od 0x08000000; opisanie oczekiwań pomaga początkującym.
- Flashowanie **uszkodzonego** lub **niepodpisanego** obrazu do 0x08010000 z zapisanym bootloaderem - bootloader powinien odmówić uruchomienia (LED3 / halt).
- Testowanie na **innej płycie** tej samej serii (na przykład NUCLEO-H743ZI bez „2”) lub innym systemie operacyjnym (Windows/macOS).

Jeśli opiszesz taki scenariusz i wynik (kroki + oczekiwania + fakt), można to uwzględnić w tym dokumencie lub w CI/skryptach.

### 3.3 Skrypty i automatyzacja

- **Skrypt „uruchom wszystkie kompilacje”** (bez oprogramowania sprzętowego): na przykład `scripts/build_all.sh`, który wywołuje `make clean`, `make bootloader`, `make app`, `make Sign-app`, `make demo`, `make -f bootloader/Makefile minimal-usart3` i wyświetla komunikat OK/FAIL.
- **Lista kontrolna w formie pliku** (np. `test/lista kontrolna.txt` lub tabela w Markdown), którą wypełniasz po ręcznych testach i dołączasz do wydania/PR.

Takie skrypty i szablony list kontrolnych można przesłać jako PR do repozytorium.

### 3.4 Dokumentacja i reprodukcja błędów

- Jeśli coś nie zgadza się z [BUILD_PL.md](BUILD_PL.md) lub z tym plikiem, zaproponuj edycję (konkretne polecenie, krok, czekaj).
- W przypadku „dziwnego” zachowania (cisza w UART, diody LED nie świecą po resecie itp.) warto wskazać: co było migane jako ostatnie, czy po flashowaniu została włączona pełna moc, czy tylko Reset, BOOT0.

---

## 5. Krótka ściągawka do poleceń

,,bicie
# Montaż wszystkiego (bez oprogramowania sprzętowego)
zrób czyste i& zrób bootloader && utwórz podpisaną aplikację && zrób demo
make -f bootloader/Makefile minimal-usart3

# Oprogramowanie sprzętowe Full Secure Boot
./scripts/flash.sh

# Samodzielne demo oprogramowania sprzętowego
utwórz demo i& st-flash zapisz build/demo/demo.bin 0x08000000

# Oprogramowanie układowe tylko dla minimalnego programu ładującego
make -f bootloader/Makefile minimal-usart3
st-flash zapis kompilacji/bootloader_minimal_usart3.bin 0x08000000
```

UART: 115200 8N1, USART3 (na NUCLEO często poprzez ST-Link VCP, na przykład `/dev/ttyACM0`).