# Montaż i oprogramowanie układowe Secure Bootloader (STM32H743ZI, NUCLEO-144)

Instrukcje krok po kroku dla tych, którzy nie są zbyt zaznajomieni z wbudowanym procesorem ARM.

---

## 1. Co należy zainstalować na swoim komputerze

### 1.1 Zestaw narzędzi ARM GCC (kompilator i linker)

- **Linux (Debian/Ubuntu):**
  ```bash
  sudo apt update
  sudo apt install gcc-arm-none-eabi
  ```
- **Windows:** pobierz [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm) i dodaj folder `bin` do PATH.
- **macOS:** `brew install arm-none-eabi-gcc`

Badanie:
```bash
arm-none-eabi-gcc --version
```

### 1.2 Python i Venv do podpisywania obrazów

Już w projekcie: `scripts/venv`. Aktywuj i sprawdź:

```bash
cd /data/projects/stm32_secure_boot
source scripts/venv/bin/activate
python -c "import ecdsa; print('ecdsa OK')"
```

### 1.3 Programator / firmware przez USB

Płytkę NUCLEO-144 podłącza się poprzez USB (ST-Link). Potrzebujesz narzędzia do zapisu do pamięci flash:

- **st-flash** (zalecane): [https://github.com/stlink-org/stlink](https://github.com/stlink-org/stlink)
Instalacja (Linux): `sudo apt install stlink-tools` lub skompiluj ze źródła.
Sprawdź: `st-info --probe` (powinien zobaczyć chip).

- Lub **STM32CubeProgrammer** (GUI) ze strony internetowej ST.

### 1.4 Płytka i zworki (NUCLEO-H743ZI2, MB1364)

**Typ płytki:** NUCLEO-H743ZI2 (wersja MB1364C) - STM32H743, 144-pin Nucleo z wbudowanym ST-LINK/V3.

**Sprawdź zworki:**

| Skoczek | Zalecenie | Miejsce docelowe |
|-----------|--------------|------------|
| **CN12** | **2-3** (przy zasilaniu tylko z USB) | Zasilanie docelowego MCU: 1-2 = ze złącza EXT, **2-3 = z USB ST-LINK**. Jeśli jest to 1-2 i EXT nie jest podłączone, MCU może nie być zasilany. |
| **CN11** | 1-2 | Sam ST-LINK zasilany jest z USB (przeważnie tak jest). |
| **JP1, JP5** | 1-2 | Ładowanie z Flasha (tryb normalny). |
| **JP6** | domyślnie 1-2 lub 2-3 - patrz UM2407 | Tryb pobierania. |
| **JP7, JP8** | domyślnie brak zworek | Niektóre tablice mają opcje; w MB1364 VCP do MCU jest zwykle już podłączone na płycie. |

**UART (VCP):** wirtualny port COM ST-LINK na tej płycie jest zwykle podłączony do **USART3 (PD8 - TX, PD9 - RX)**. Domyślne oprogramowanie wykorzystuje USART2 (PA2/PA3); jeśli nie ma żadnych logów, skompiluj z `-DUART_LOG_USE_USART3` (patrz sekcja 6.1).

**Oficjalna dokumentacja:** [Instrukcja użytkownika UM2407](https://www.st.com/resource/en/user_manual/um2407-stm32h7-nukleo144-boards-mb1364-stmicroelectronics.pdf) (płyty STM32H7 Nucleo-144 MB1364).

---

## 2. Karta pamięci (jak wszystko jest w pamięci flash)

| Adres | Spis treści |
|------------|------------|
| 0x08000000 | Program ładujący, 64 KB |
| 0x08010000 | Nagłówek obrazu (image_header_t, 96 bajtów) |
| 0x08010060 | Kod aplikacji (tabela wektorów + program) |

Podczas uruchamiania bootloader odczytuje nagłówek pod adresem 0x08010000, odczytuje SHA-256 kodu aplikacji, sprawdza podpis ECDSA i, jeśli się powiedzie, przechodzi do adresu z pola `entry_point` (0x08010060).

---

## 3. Montaż bootloadera

Z katalogu głównego projektu:

```bash
cd /data/projects/stm32_secure_boot
make
```

Lub wyraźnie:

```bash
make -f bootloader/Makefile all
```

Wynik:
- `build/bootloader.elf` — obraz do debugowania;
- `build/bootloader/bootloader.bin` - plik binarny do flashowania oprogramowania układowego do obszaru 0x08000000.

Domyślnie włączony jest **tryb pośredni** (makro `USE_ECDSA_STUB`): podpis nie jest sprawdzany, każdy obraz jest uznawany za „prawdziwy”. W ten sposób możesz szybko sprawdzić łańcuch: bootloader → przejdź do aplikacji. Weryfikacja prawdziwego podpisu wymaga kompilacji z mbedTLS (patrz sekcja 7).

---

## 4. Budowa aplikacji

```bash
make -f app/Makefile all
```

Wynik: `build/app/app.bin` - surowy obraz aplikacji (bez nagłówka), skompilowany dla adresu 0x08010060.

---

## 5. Podpisanie obrazu aplikacji

Skrypt dodaje nagłówek z podpisem do pliku `app.bin` i tworzy pełny obraz wpisu zaczynając od 0x08010000:

```bash
source scripts/venv/bin/activate
python scripts/sign_image.py build/app/app.bin build/app/signed_app.bin
```

Plik `scripts/root_private_key.pem` musi istnieć (jeśli klucze jeszcze nie istnieją, zobacz `scripts/generate_keys.py` i zaktualizuj `bootloader/inc/keys.h` kluczem publicznym).

---

## 6. Oprogramowanie sprzętowe na płycie

1. Podłącz NUCLEO-144 przez USB.
2. Zapisz bootloader pod adresem 0x08000000:
   ```bash
   st-flash write build/bootloader/bootloader.bin 0x08000000
   ```
3. Napisz podpisany wniosek na adres 0x08010000:
   ```bash
   st-flash write build/app/signed_app.bin 0x08010000
   ```

Lub za pomocą jednego polecenia (najpierw bootloader, potem aplikacja w jeden połączony plik - możesz to przygotować osobnym skryptem).

Po zresetowaniu płytki powinien uruchomić się bootloader, następnie przejść do aplikacji. W NUCLEO-144 zielona dioda LED1 (PB0) powinna migać. Jeśli podpis się nie powiedzie (podczas budowania bez kodu pośredniczącego i z mbedTLS), zaświeci się czerwona dioda LD3 (PB14), a płyta „zawiesi się” w bootloaderze.

**Zaleca się użycie skryptu** `./scripts/flash.sh` - zbiera jeden obraz i flashuje go od 0x08000000 (w STM32H7 strona flash ma 128 KB, zapisuje tylko od krawędzi strony).

---

## 6.1 Logi poprzez UART (konsola)

Bootloader i aplikacja wysyłają tekst do **wirtualnego portu COM** ST-Link (USART3, PD8/PD9, **115200 8N1**). Nie są potrzebne żadne dodatkowe przewody – port jest już podłączony do złącza płytki.

1. Podłącz NUCLEO przez USB (ten sam kabel co do oprogramowania).
2. W systemie pojawi się port szeregowy (na przykład `/dev/ttyACM0` w systemie Linux).
3. Otwórz terminal za pomocą **115200 8N1** (wymagana prędkość):
   ```bash
   minicom -D /dev/ttyACM0 -b 115200
   ```
lub `screen /dev/ttyACM0 115200`. W Minicom domyślną wartością może być 9600 - sprawdź: **Ctrl+A O** → Konfiguracja portu szeregowego → **E** (Bps/Par/Bits) → 115200 8N1.
4. Najpierw otwórz terminal, **następnie** naciśnij **Reset** na płycie - w oknie powinny pojawić się następujące linie:
   ```
   [boot] Secure bootloader
   [boot] UART 115200 OK
   Verify: header...
     magic OK
     size OK
     SHA256...
     SHA256 OK
     ECDSA verify...
     ECDSA OK
   [boot] Signature OK
   [boot] Jump to app
   App started
   ```

Jeśli wystąpi błąd podpisu, program ładujący wyświetli komunikat „[boot] Signature FAIL, halt” (oraz jeden z komunikatów „FAIL: bad magic”, „FAIL: image_size=0”, „FAIL: SHA256”, „FAIL: ECDSA”), zaświeci się czerwona dioda LD3.

Domyślnie bootloader jest montowany z **USART3 (PD8/PD9)** dla dziennika - ST-Link VCP na NUCLEO-H743ZI2.

### Minimalny program ładujący do sprawdzania dziennika

Jeśli chcesz się upewnić, że wyjście do UART działa bez sprawdzania podpisu i przechodzenia do aplikacji:

1. Zbuduj minimalny bootloader (tylko inicjalizacja UART i kilka linii w logu):
   ```bash
   make minimal
   ```
2. Flashuj go pod adresem 0x08000000:
   ```bash
   st-flash write build/bootloader/bootloader_minimal.bin 0x08000000
   ```
3. Otwórz terminal 115200 8N1 i kliknij Resetuj. Powinny pojawić się linie:
   ```
   === Bootloader log test ===
   If you see this, UART is OK.
   115200 8N1
   ```
4. Aby odzyskać pełny bootloader i aplikację, uruchom ponownie `./scripts/flash.sh`.

### Samodzielne demo: diody LED + log (bez bootloadera)

W głównym katalogu projektu znajduje się minimalny obraz **demo**: trzy diody LED (LED1/LED2/LED3) migają na zmianę, a ich stan jest wyświetlany w UART.

- **Montaż:**
  ```bash
  make demo
  ```
Wynik: `build/demo/demo.bin`.

- **Firmware** (obraz jest zapisany od 0x08000000, bootloader nie jest używany):
  ```bash
  st-flash write build/demo/demo.bin 0x08000000
  ```

- **UART:** USART3 (PD8/PD9), 115200 8N1 - ten sam wirtualny port COM ST-Link. W logu jedna po drugiej znajdują się takie linie jak:
  ```
  Demo: LEDs + log (NUCLEO-H743ZI)
  LED1=ON LED2=OFF LED3=OFF
  LED1=OFF LED2=ON LED3=OFF
  LED1=OFF LED2=OFF LED3=ON
  ```

Więcej szczegółów: `app/demo/README`.

### Oficjalne oprogramowanie do testowania UART (opcjonalnie)

Aby upewnić się, że płyta i wirtualny port COM działają, możesz sflashować **oficjalny przykład** z pakietu STM32CubeH7:

1. Pobierz [STM32CubeH7](https://github.com/STMicroelectronics/STM32CubeH7/releases) lub sklonuj repozytorium.
2. Otwórz przykładowy projekt UART/COM dla NUCLEO-H743ZI, na przykład:
- `STM32CubeH7/Projects/NUCLEO-H743ZI/Examples/UART/` (jeśli istnieje UART_Printf lub odpowiednik),
- lub dowolne BSP/Przykłady, w których włączone jest wyjście COM (115200).
3. Zbuduj projekt w STM32CubeIDE lub używając make/CMake z pakietu i wgraj wynikowy plik `.bin` pod adresem **0x08000000**:
   ```bash
   st-flash write <path-to-built>.bin 0x08000000
   ```
4. Otwórz minicom/screen pod adresem 115200 i kliknij Resetuj – w terminalu powinny pojawić się dane wyjściowe z przykładu ST.

To sprawdzi, czy zasilacz, zworki i VCP na płycie są w porządku. Następnie możemy ponownie sflashować nasz obraz za pomocą `./scripts/flash.sh`.

---

Teraz domyślnie bootloader jest zbudowany z `-DUSE_ECDSA_STUB` i nie sprawdza podpisu. Aby włączyć weryfikację:

1. Pobierz do projektu mbedTLS, np.:
   ```bash
   git clone --depth 1 https://github.com/Mbed-TLS/mbedtls.git third_party/mbedtls
   ```
2. Zbuduj mbedTLS tylko z niezbędnymi modułami (ECP, ECDSA, Bignum) i pobierz bibliotekę (na przykład `libmbedtls.a`) lub dodaj źródła do zestawu bootloadera.
3. W `bootloader/Makefile` usuń flagę `-DUSE_ECDSA_STUB` i dodaj ścieżki do mbedTLS („-I Third_party/mbedtls/include`) oraz link do `libmbedtls.a`.

Następnie bootloader sprawdzi podpis; Jeżeli podpis będzie nieprawidłowy, zaświeci się czerwona dioda LD3 i nie będzie możliwości przejścia do aplikacji.

---

## 8. Krótka lista kontrolna

1. Zainstaluj `arm-none-eabi-gcc`, aktywuj `scripts/venv`, w razie potrzeby zainstaluj `st-link`.
2. Zbuduj bootloader: `make`.
3. Zbuduj aplikację: `make -f app/Makefile all`.
4. Podpisz obraz: `python scripts/sign_image.py build/app/app.bin build/app/signed_app.bin`.
5. Flash: `st-flash zapis build/bootloader/bootloader.bin 0x08000000`, następnie `st-flash zapis build/app/signed_app.bin 0x08010000`.
6. Zresetuj płytkę - powinna migać na zielono (aplikacja uruchomiła się po bootloaderze).

Jeśli coś nie zostało zmontowane lub nie zostało sflashowane, wyślij tekst błędu i wynik polecenia.
