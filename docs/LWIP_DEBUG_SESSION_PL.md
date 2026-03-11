# Debuguj sesję lwip: uruchom pod GDB i znajdź przyczynę niepowodzenia

Jeden scenariusz, od włączenia zarządu do ustalenia, gdzie dokładnie następuje przerwa w startupie (i praktyka GDB).

---

## Przygotowanie

- Płyta **NUCLEO-H743ZI2** przez USB (ST-Link).
- Dwa terminale (lub zakładki). Jeden będzie zawierał `st-util`, drugi będzie zawierał GDB.

---

## Część 1. Uruchamianie w debugerze

### Krok 1. Terminal 1 – serwer GDB

```bash
st-util
```

Zostaw okno otwarte. Powinno brzmieć: `Słucham o *:4242...`

---

### Krok 2: Terminal 2 — zbuduj i uruchom GDB

```bash
cd /path/to/stm32_secure_boot
make lwip
./scripts/gdb_lwip.sh
```

Skrypt połączy się ze st-util, załaduje obraz na płytkę, wykona reset+halt i ustawi punkt przerwania na `main`. Na końcu pojawi się zachęta `(gdb)`.

---

### Krok 3. Pierwsze uruchomienie

W GDB wpisz:

```
(gdb) c
```

**Co może być:**

- **Zatrzymany na `main`** - uruchomienie na main powiodło się, błąd w dalszej części kodu (lub nie istniał). Możesz umieścić punkt przerwania w `StartDefaultTask`, `log_puts` itp.
- **Zatrzymany przy `UsageFault_Handler`** (lub innym błędzie) - awaria przed lub wkrótce po wejściu do pliku main. Przejdź do **Części 2**.

---

## Część 2. Dotarliśmy do UsageFault_Handler - szukamy przyczyny

Nie opuszczając GDB, wykonuj po kolei.

### 2.1 Znajdź przyczynę niepowodzenia (CFSR)

```
(gdb) p/x *(uint32_t*)0xE000ED28
```

Lub po przebudowie za pomocą naszego kodu:

```
(gdb) p/x g_usage_fault_cfsr
```

Zobacz **wysokie 16 bitów** (UFSR). Typowe wartości:

| Znaczenie | Powód |
|-----------|---------|
| 0x0100 | UNDEFINSTR - niepoprawna/nieobsługiwana instrukcja |
| 0x0200 | INVPC — Nieprawidłowy komputer przy wyjściu z wyjątku |
| 0x0400 | INVSTATE - nieprawidłowy stan (np. przejście do Thumb z dziwnego adresu) |
| 0x0800 | UNALIGNED - niewyrównany dostęp do pamięci |
| 0x1000 | DIVBYZERO - dzielenie przez zero |

Zapisz wartość (na przykład „0x400”).

---

### 2.2 Znajdź adres nieudanej instrukcji

Komórka **at** `$sp+24` zawiera **wartość** PC (adres instrukcji, która się nie powiodła). Najpierw przeczytaj tę wartość:

```
(gdb) x/xw $sp+24
```

Należy pamiętać, że **wydrukowana wartość** (np. `0x08001234`) to adres instrukcji, która się nie powiodła. Następnie zdemontuj **dokładnie to** (nie adres komórki stosu):

```
(gdb) x/i 0x08001234
```

(wpisz swój adres). Polecenie `x/i $sp+24` byłoby niepoprawne - rozkłada adres komórki stosu w pamięci RAM, a nie kod z zapisanego komputera. GDB wyświetli instrukcję oraz, jeśli występują symbole, plik i linię. W ten sposób będziesz wiedzieć, **gdzie** wystąpiła awaria.

**Jeśli wartość `$sp+24` wynosi 0:** jest to powrót na adres 0 - zwykle oznacza to, że 0 jest zapisane w tablicy wektorów dla jakiegoś przerwania, a kiedy przerwanie zostało wyzwolone, procesor przeszedł na adres 0 (INVPC). Podczas uruchamiania należy wypełnić sloty IRQ procedurami obsługi (na przykład `Default_Handler`), a dla Ethernetu (ETH_IRQn = 61) - `ETH_IRQHandler`.

---

### 2.3 Zresetuj i uruchom ponownie (bez ponownego uruchamiania GDB)

```
(gdb) monitor reset halt
(gdb) c
```

- Jeśli ponownie trafisz na usterkę, powtórz kroki 2.1 i 2.2 (powód i adres mogą zostać zaktualizowane).
- Aby zawęzić miejsce: przed `c` umieść punkt przerwania wcześniej w kodzie (patrz część 3).

---

## Część 3. Zawężanie lokalizacji awarii (praktyka GDB)

Cel: zrozumieć, gdzie sięga egzekucja, a gdzie już spada.

### Opcja A: Niepowodzenie przed wykonaniem głównego

Umieść punkt na samym początku resetu i postępuj zgodnie z instrukcjami:

```
(gdb) monitor reset halt
(gdb) break Reset_Handler
(gdb) c
```

Zatrzymaj się w `Reset_Handler`. Dalej:

- **`n`** (next) — wykonaj jedną instrukcję na raz (bez wpisywania funkcji).
- Po kilku `n` dojdziesz do `bl ExitRun0Mode`, następnie `bl SystemInit`, następnie .bss/.data pętli, a następnie `bl main`. Jeśli podczas jednego z `bl` pojawi się awaria, oznacza to awarię **w ramach** tej funkcji.
- **s`** (krok) — krok z wejściem do funkcji. Użyj, stojąc na `bl...`, jeśli chcesz wejść do środka.

Zdrowy:

```
(gdb) info registers pc
(gdb) disas
```

`disas` pokaże deasembler bieżącej funkcji.

### Opcja B: Sprawdź, czy dotarliśmy do głównego

```
(gdb) monitor reset halt
(gdb) break main
(gdb) c
```

- **Zatrzymany na `main`** - błąd gdzieś **po** wejściu do main (lub w samym main). Umieść punkt przerwania w `log_init`, `log_puts`, `MPU_Config`, `SystemClock_Config` i `c` ponownie, aby znaleźć linię.
- **Uderz ponownie UsageFault_Handler** - błąd **przed** main (uruchamianie, ExitRun0Mode, SystemInit lub kopiowanie .data/.bss). Następnie użyj Opcji A z `break Reset_Handler`.

### Opcja C: Punkt przerwania w określonej funkcji

Na przykład sprawdź, czy inicjalizacja UART nazywa się:

```
(gdb) break log_init
(gdb) monitor reset halt
(gdb) c
```

Jeśli zatrzymałeś się w `log_init`, wykonanie osiągnie ten punkt. Następnie możesz wpisać „n” linia po linii w `log_init` lub ponownie wstawić przerwę w `log_puts` i `c`.

---

## Część 4. Przydatne polecenia GDB (ściągawka)

| Zespół | Skrót | Opis |
|---------|------------|----------|
| „kontynuuj” | `c` | Wykonuj do następnego punktu przerwania lub błędu |
| `następny` | `n` | Następna linia (bez wpisu funkcji) |
| „krok” | `s` | Następna linia (z wejściem do funkcji) |
| `przerwa <funkcja>` | `b główny` | Punkt przerwania funkcji |
| `break <plik>:<linia>` | `b main.c:72` | Punkt przerwania linii |
| `informacyjne punkty przerwania` | | Lista punktów przerwania |
| `usuń 1` | | Usuń punkt przerwania nr 1 |
| `wydrukuj wyrażenie` | `zmienna p/x` | Drukuj wartość (`x` = szesnastkowo) |
| „śledzenie wsteczne” | `bt` | Stos połączeń |
| `lista` | `l` | Pokaż kod źródłowy wokół bieżącej linii |
| `disas` | | Deasembler bieżącej funkcji |
| `ładuj` | | Wgraj obraz na tablicę (po ponownym złożeniu) |
| `zatrzymanie resetowania monitora` | | Zresetuj planszę i zatrzymaj się na początku |
| „wyjdź” | `q` | Wyjdź z GDB |

---

## Część 5. Po zmianie kodu

Przebudowałem projekt:

```bash
make lwip
```

W **tym samym** GDB (nie trzeba restartować):

```
(gdb) load
(gdb) monitor reset halt
(gdb) c
```

Lub umieść niezbędne punkty przerwania i ponownie „c”.

---

## Streszczenie

1. **Terminal 1:** `st-util`.
2. **Terminal 2:** `make lwip` → `./scripts/gdb_lwip.sh` → `c`.
3. Jeśli trafisz **UsageFault_Handler**: zobacz CFSR (`p/x *(uint32_t*)0xE000ED28`), błędny adres (`x/xw $sp+24`, następnie `x/i <adres>`).
4. Zawęź miejsce: `break Reset_Handler` lub `break main`, następnie `monitor reset halt` i `c`; krok `n`/`s`, jeśli to konieczne.
5. Nie uruchamiaj ponownie GDB; po `make lwip` wykonaj `load`, `monitor reset halt`, `c` w nim.

Korzystając z tego scenariusza, możesz zarówno znaleźć przyczynę problemów z uruchamianiem, jak i przećwiczyć korzystanie z GDB.
