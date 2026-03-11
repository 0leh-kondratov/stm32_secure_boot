# Jak sprawdzić, dlaczego aplikacja/lwip nie działa

Implementacja app/lwip jest podana w oficjalnym przykładzie ST **LwIP HTTP Server Netconn RTOS** (katalog `stm32_eth_files`): te same regiony MPU dla sterty ETH DMA i LwIP, rozmieszczenie sekcji w D2 SRAM (0x30000000), unieważnienie D-Cache przy odbiorze, przepływ kontroli łącza i DHCP.

## 1. Podłącz UART i otwórz terminal

- **Port:** w NUCLEO-H743ZI dziennik przechodzi przez **USART3** (PD8/PD9), często przesyłany dalej przez **ST-Link VCP**. W systemie Linux portem jest zwykle `/dev/ttyACM0` lub `/dev/ttyUSB0`.
- **Prędkość:** 115200 8N1.

Znajdź port i otwórz terminal:

```bash
# Znajdź port (po podłączeniu płytki przez USB)
ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
# Lub
st-info --probe
```

Otwórz terminal (zastąp swój port):

```bash
minicom -D /dev/ttyACM0 -b 115200
# Lub
screen /dev/ttyACM0 115200
```

W minicom: **Ctrl+A O** → Konfiguracja portu szeregowego → **E** (Bps/Par/Bity) → 115200 8N1.

**Przed flashowaniem lwip** otwórz terminal i pozostaw go otwartym, a następnie zresetuj płytkę (przycisk lub zasilanie).

---

## 2. Co powinieneś zobaczyć krok po kroku

Jeśli się powiedzie, pojawi się UART **w tej kolejności**:

| Co widzisz | Znaczenie |
|------------|----------|
| **`lwip: boot`** | Płyta uruchomiła się, zegary i UART działają. Jeśli tak nie jest, problem leży po stronie harmonogramu (zegar, port, oprogramowanie w złym miejscu). |
| **`aplikacja/lwip: FreeRTOS + Ethernet`** | Zadanie StartDefaultTask zostało uruchomione, dziennik został zainicjowany. |
| **`--- TCP/IP ---`** i dalsze IP/maska ​​sieci/brama/MAC | LwIP i sieć działają. W przypadku DHCP na początku może pojawić się „Łącze wyłączone (brak IP)”, po kilku sekundach – adres. |

Dodatkowo: **LED1 (PB0)** powinna **świecić** natychmiast po resecie (włączona na samym początku `main()`); po uruchomieniu harmonogramu zacznie on **migać** mniej więcej raz na 0,5 sekundy.

---

## 3. Według objawów: co sprawdzić

### W UART nie ma zupełnie nic

1. **Sprawdź LED1 (PB0):** w `main()` pierwsza rzecz, jaką LED1 zapala się w rejestrach (przed MPU i HAL).
- **LED1 świeci** - wykonanie osiąga `main()`, miga obraz lwip. Wtedy problem występuje pomiędzy wczesną diodą LED1 a UART: `HAL_Init()`, `SystemClock_Config()` lub `log_init()`/`log_puts()`. Sprawdź, czy terminal jest na **115200**, porcie **USART3** (często przez ST-Link VCP - `/dev/ttyACM0`).
- **LED1 nie świeci** - nie osiągnęliśmy `main()` lub wyświetlił się niewłaściwy obraz. Uruchom `make flash-lwip` i zresetuj płytę. Upewnij się, że oprogramowanie sprzętowe jest zainstalowane w **0x08000000** (nie w obszarze bootloadera).
2. **Upewnij się, że UART i płyta są pod napięciem:** wykonaj flash step1 i otwórz ten sam port 115200:
   ```bash
   make step1 && make flash-step1
   ```
Powinny pojawić się linie „Krok 1: OK”. Jeśli krok 1 również milczy, spójrz na port (ttyACM0/ttyUSB0), 115200, BOOT0=0, zasilanie.
3. **Po kroku 1 ponownie wykonaj flashowanie lwip** i zresetuj płytę - sprawdź, czy pojawia się przynajmniej `lwip: boot`.

### Jest tylko „lwip: boot”, a potem cisza

Oznacza to zawieszenie w harmonogramie lub zadaniu do czasu pierwszego „log_puts”. Możliwe miejsca: `osKernelStart()`, początek `StartDefaultTask` przed `log_init()` (wywołany po raz drugi w zadaniu - nic wielkiego) lub upuść przed wejściem do zadania. Warto zajrzeć do debugera (GDB + st-util), umieścić punkt przerwania w `StartDefaultTask`.

### Istnieje `app/lwip: FreeRTOS + Ethernet`, ale nie ma bloku TCP/IP

Zawieszenie lub awaria podczas inicjalizacji LwIP/sieci: `tcpip_init()`, `Netif_Config()` lub oczekiwanie na łącze/DHCP. Sprawdzać:

- Kabel Ethernet jest podłączony do płytki i do aktywnego portu (router/komputer).
- Wskaźniki łącza na porcie (jeśli występują).

Przy statycznym adresie IP (jeśli później przełączysz się na statyczny w kodzie), blok TCP/IP pojawi się natychmiast po podniesieniu interfejsu.

### Dioda LED1 nie miga

Albo program planujący nie osiąga zadania LED1, albo zadanie nie zostało utworzone. Według logu: jeśli jest tam „app/lwip: FreeRTOS + Ethernet”, zadanie zostało rozpoczęte; jeśli występuje blok TCP/IP, oznacza to, że kod dociera do diody LED1. Jeśli dioda LED1 nadal nie miga, sprawdź pin (PB0 na tej płytce).

---

## 4. Debugowanie w GDB (opcjonalnie)

1. Uruchom serwer: `st-util`.
2. W innym terminalu:
   `arm-none-eabi-gdb build/lwip/lwip.elf`  
następnie: `docelowy rozszerzony-zdalny :61234`, `ładuj`, `kontynuuj`.
3. W Cursor/VS Code możesz użyć konfiguracji dla „Debug demo (ST-Link, sprzęt)”, zastępując ścieżkę do `build/lwip/lwip.elf`.

Możesz więc umieścić punkt przerwania w `main`, w `StartDefaultTask`, w `log_tcpip_params` i zobaczyć, w którym kroku wykonywanie się zatrzymuje.

---

## 5. Krótka lista kontrolna

- [ ] Płyta NUCLEO-H743ZI, BOOT0 = 0, zasilana przez USB.
- [ ] Oprogramowanie sprzętowe: `make lwip && make flash-lwip`.
- [ ] Po zresetowaniu **LED1 świeci** (włączona na początku głównego; jeśli się nie zaświeci, obraz nie zostanie uruchomiony).
- [ ] Terminal otwarty na właściwym porcie (często `/dev/ttyACM0`), 115200 8N1.
- [ ] W terminalu widzę co najmniej `lwip: boot`.
- [ ] Podłączony Ethernet; z DHCP, poczekaj kilka sekund, aż adres IP pojawi się w dzienniku.

Jeśli krok 1 przez UART działa, a po flashowaniu lwip nie ma nawet `lwip: boot`, oznacza to, że obraz lwip nie uruchamia się (adres firmware, wektor resetowania lub zawiesza się do pierwszego wyjścia).
