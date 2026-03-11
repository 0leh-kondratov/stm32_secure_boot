# Debugowanie NUCLEO-H743ZI2

W skrócie: jak podłączyć debugger do płytki NUCLEO-H743ZI2 (wbudowany ST-Link), przyjrzeć się kodowi krok po kroku i ustawić breakpointy.

**Pełny skrypt na jedną sesję** (uruchomienie lwip pod GDB + poszukiwanie przyczyny awarii + ćwiczenie GDB): **[LWIP_DEBUG_SESSION_PL.md](LWIP_DEBUG_SESSION_PL.md)**.

---

## Kolejność uruchomienia testu (lwip na NUCLEO-H743ZI2)

| Nie. | Gdzie | Akcja |
|---|-----|----------|
| 1 | Terminal 1 | Uruchom `st-util` i poczekaj na `Słuchanie o *:4242...` |
| 2 | Terminal 2 | `cd` do katalogu projektu, uruchom `make lwip` |
| 3 | Terminal 2 | Uruchom `./scripts/gdb_lwip.sh` |
| 4 | GDB | Wpisz `c` (kontynuuj) - zatrzymaj się na `main` lub w procedurze obsługi błędów |
| 5 | Dalej | Ustaw punkty przerwania, krok (`n`/`s`), spójrz na zmienne; przy zmianie kodu - `load`, `monitor reset halt`, `c` |

Opcjonalnie: dziennik UART (minicom/screen na `/dev/ttyACM0` lub `/dev/ttyACM1`, 115200 8N1) - aby zobaczyć wynik `log_puts` po wejściu do pliku main.

---

## 1. Czego potrzebujesz

- **Płyta** NUCLEO-H743ZI2, podłączana przez USB (ST-Link).
- **stlink-tools** - `st-util` (serwer GDB), `st-flash` (firmware):
  ```bash
  sudo apt install stlink-tools   # Linux
  ```
Lub zbuduj: [stlink-org/stlink](https://github.com/stlink-org/stlink).
- **arm-none-eabi-gdb** (często występuje z `gcc-arm-none-eabi`).

Opcjonalnie w Cursor/VSCode: rozszerzenie **Cortex-Debug** (marus25.cortex-debug) - wtedy możesz rozpocząć debugowanie za pomocą F5 bez ręcznego `st-util`.

---

## 2. Schemat ogólny

1. Zbuduj oprogramowanie **z symbolami debugowania** (plik Makefile ma już `-g` dla lwip/step1; dla bootloadera - `make -f bootloader/Makefile debug`).
2. W **pierwszym terminalu** uruchom serwer GDB: `st-util` (nasłuchuje na porcie **4242**).
3. W **drugim terminalu** uruchom GDB z wymaganym `.elf`, połącz się ze st-util, załaduj obraz jeśli to konieczne i ustaw punkty przerwania.

---

## 3. Debugowanie aplikacji lwip

### Krok po kroku (od zera)

**Krok 1.** Otwórz dwa terminale. W **terminalu 1** uruchom serwer GDB i nie zamykaj go:

```bash
st-util
```

Powinien pojawić się komunikat „Słuchanie o *:4242...”.

**Krok 2.** W **terminalu 2** zbuduj lwip i uruchom GDB:

```bash
cd /path/to/stm32_secure_boot
make lwip
./scripts/gdb_lwip.sh
```

Sam skrypt: łączy się z st-util, ładuje obraz (`load`), resetuje i zatrzymuje, umieszcza punkt przerwania na `main` i zatrzymuje się na znaku zachęty `(gdb)`.

**Krok 3.** W GDB naciśnij **`c`** (lub `kontynuuj`). Wykonanie osiągnie `main` i zatrzyma się. Następnie możesz ustawić inne punkty przerwania, krok (`n`/`s`), spójrz na zmienne.

**Nie ma potrzeby ponownego uruchamiania GDB** – łączysz się raz i pracujesz w tej samej sesji. Jeśli przebudowałeś projekt (`make lwip`), w tym samym GDB uruchom ponownie `load` i, jeśli to konieczne, `monitor reset halt`, a następnie `c`.

---

### Jeśli zatrzymany w UsageFault_Handler (krok po kroku, bez ponownego uruchamiania GDB)

Jesteś już w GDB i działasz w `UsageFault_Handler`. Zrób to w kolejności **w tej samej sesji GDB**:

| Krok | Zespół w GDB | Dlaczego |
|-----|----------------|-------|
| 1 | `p/x *(uint32_t*)0xE000ED28` | Przyczyna niepowodzenia (CFSR). Bity 16-31 = UFSR (0x0100=nieprawidłowa instrukcja, 0x0400=INVSTATE, 0x0800=dostęp niewyrównany, 0x1000=dzielenie przez zero). |
| 2 | `x/xw $sp+24` | Adres instrukcji, która się nie powiodła (PC ze stosu). Zapamiętaj numer (na przykład 0x08001234). |
| 3 | `x/i 0x08001234` | Zastąp swój adres z kroku 2. Wyświetli samą instrukcję i, jeśli są znaki, plik:string. |
| 4 | `zatrzymanie resetowania monitora` | Zresetuj planszę i zatrzymaj się na początku. |
| 5 | `c` | Początek. Albo osiągnie punkt przerwania na `main`, albo ponownie wylądujesz w UsageFault_Handler. |

Jeśli ponownie naciśniesz UsageFault_Handler, powtórz kroki 1–3 (po przebudowaniu będzie już zaktualizowany `g_usage_fault_cfsr`). Aby zawęzić lokalizację awarii, przed krokiem 5 umieść punkt przerwania wcześniej w kodzie, na przykład:

- `break Reset_Handler` - następnie `c`, następnie w krokach `n` (następny);
- lub `break main` i `c` - jeśli osiągnie main, to zawiesza się po wejściu do main.

**Przebudowałeś kod?** W tym samym GDB: `load`, następnie `monitor reset halt`, a następnie `c`. Nie ma potrzeby ponownego uruchamiania GDB.

---

### Montaż (pomoc)

```bash
make lwip
```

Kompiluj już z `-g`, symbole w `build/lwip/lwip.elf`.

### Terminal 1 – serwer GDB

```bash
st-util
```

Pozostaw uruchomiony (powinno być „Słuchanie o *:4242”).

### Terminal 2 – GDB

```bash
cd /path/to/stm32_secure_boot
./scripts/gdb_lwip.sh
```

Lub ręcznie:

```bash
arm-none-eabi-gdb build/lwip/lwip.elf
```

W GDB:

```
(gdb) target extended-remote localhost:4242
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

Dalsze wykonywanie zatrzyma się na `main`. Możesz ustawić punkty przerwania, na przykład:

- `break main` - wejście do programu
- `break StartDefaultTask` - wprowadź zadanie FreeRTOS
- `break log_puts` - każde wyjście w UART
- `break ethernetif_init` — inicjalizacja Ethernetu

Polecenia: `kontynuuj` (c), `następny` (n), `krok` (s), `drukuj zmienną`, `backtrace` (bt).

---

## 4. Debugowanie krok 1 / krok 2

Podobnie: build (`zrób krok 1` lub `zrób krok 2`), w jednym terminalu `st-util`, w innym:

```bash
arm-none-eabi-gdb build/step1/step1.elf
# lub build/step2/step2.elf
(gdb) target extended-remote localhost:4242
(gdb) load
(gdb) monitor reset halt
(gdb) break main
(gdb) continue
```

---

## 5. Debugowanie bootloadera

Szczegóły: [DEBUG_PL.md](DEBUG_PL.md).

W skrócie: zbuduj za pomocą debugowania `make -f bootloader/Makefile debug`, następnie `st-util` i:

```bash
./scripts/gdb_bootloader.sh
```

lub ręcznie za pomocą `build/bootloader/bootloader.elf`.

---

## 6. Debugowanie za pomocą kursora/VSCode

### Opcja A: wbudowany debugger C/C++ (GDB + st-util)

1. Uruchom **st-util** w oddzielnym terminalu.
2. W Uruchom i debuguj wybierz konfigurację **Debuguj lwip (GDB + st-util)** (jeśli jest dostępna w `.vscode/launch.json`).
3. F5 - połącz się ze st-util, załaduj elfa, zatrzymaj się na `main`.

Ścieżka do GDB w launch.json: `miDebuggerPath` - na przykład `arm-none-eabi-gdb` lub pełna ścieżka, jeśli nie jest w PATH.

### Opcja B: Debugowanie Cortex

Zainstaluj rozszerzenie **Cortex-Debug**. Może sam uruchomić st-util/openocd i połączyć GDB - konfiguracja w launch.json z `"servertype": "stutil"` i ścieżką do elf.

---

## 7. Typowe problemy

| Problem | Rozwiązanie |
|----------|---------|
| Odmowa połączenia pod numerem 4242 | Uruchom `st-util` w oddzielnym terminalu i nie zamykaj go. |
| Żadna tabela symboli / nazwy funkcji nie są widoczne | Kompiluj z `-g` (dla lwip - `make lwip` już z `-g`). |
| Po załadowaniu kod jest „zły” | Flashuj bieżący obraz (`make flash-lwip`, itp.), następnie w GDB ponownie `load` i `monitor reset halt`. |
| Tablica nie jest widoczna (st-util jej nie znajduje) | Sprawdź kabel USB i port; `lsusb` (musi to być łącze ST); w razie potrzeby podłącz ponownie płytkę. |

---

## 8. Przydatne punkty przerwania (lwip)

- **Reset_Handler** (przy uruchomieniu) - pierwszy kod po resecie.
- **main** — po inicjalizacji podczas uruchamiania.
- **log_init** / **log_puts** — wyjście do UART.
- **StartDefaultTask** — początek zadania, w którym wywoływane są tcpip_init, Netif_Config.
- **ethernetif_init** / **low_level_init** — inicjalizacja Ethernetu.
- **HAL_ETH_MspInit** — konfiguracja pinów i przerwań ETH.

Jeśli po resecie nie ma UART i LED1, ustaw punkt przerwania w **Reset_Handler** i sprawdź, czy wykonanie osiągnie włączenie LED1 i `main`.

---

## 9. Zatrzymano w UsageFault_Handler

Jeśli podczas łączenia z GDB lub po „kontynuuj” wykonanie zakończy się w **UsageFault_Handler**, wystąpił błąd użycia (nieprawidłowa instrukcja, niewyrównany dostęp, dzielenie przez zero itp.).

**W GDB wykonaj:**

1. **Przyczyna awarii (CFSR/UFSR):**
   ```
   (gdb) p/x g_usage_fault_cfsr
   ```
Bity 16–31 to UFSR. Typowe wartości (ARM Cortex-M7):
- `0x0100` - UNDEFINSTR (nieprawidłowa instrukcja)
- `0x0200` - INVPC (nieprawidłowy komputer w przypadku wyjątku)
- `0x0400` — INVSTATE (nieprawidłowy stan, na przykład przejście do Thumb z nieparzystym adresem)
- `0x0800` - UNALIGNED (dostęp niewyrównany)
- `0x1000` — DIVBYZERO (dzielenie przez zero)

2. **Adres nieudanej instrukcji** (PC na stosie przy wejściu do procedury obsługi):
   ```
   (gdb) x/xw $sp+24
(gdb) x/i <adres_otrzymany>
   ```
Pod adresem możesz znaleźć linijkę w kodzie lub zajrzeć do dezasemblera.

3. **Zresetuj i powtórz:**
   ```
   (gdb) monitor reset halt
   (gdb) break main
   (gdb) continue
   ```
Jeśli ponownie ulegnie awarii w UsageFault_Handler, umieść punkt przerwania w **Reset_Handler**, następnie **ExitRun0Mode**, **SystemInit**, **main** i krok po kroku („next”), aby zawęzić lokalizację awarii.
