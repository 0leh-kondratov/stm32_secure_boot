# STM32 Secure Boot + FreeRTOS/LwIP (NUCLEO-H743ZI2)

Repozytorium zawiera zestaw firmware'ow edukacyjnych i testowych dla STM32H743:
- secure bootloader,
- kilka etapow aplikacji (`step1`, `step2`),
- profile sieciowe z FreeRTOS + LwIP (`lwip`, `lwip_zero`),
- scenariusze testow i debugowania.

Projekt jest nastawiony na praktyczne uruchamianie na plytce **NUCLEO-H743ZI2** oraz szybka diagnostyke przez UART/GDB.

---

## Co znajdziesz w repo

- `bootloader/` - secure boot (weryfikacja obrazu aplikacji przed startem).
- `app/step1/` - prosty profil testowy: LED + UART.
- `app/step2/` - FreeRTOS + logger + I2C scanner.
- `app/lwip/` - starszy profil LwIP.
- `app/lwip_zero/` - aktualny profil "od zera": FreeRTOS + LwIP + Netconn HTTP + logi UART.
- `common/` - wspolne moduly, m.in. logger UART.
- `docs/` - dokumentacja techniczna i notatki uruchomieniowe.
- `scripts/` - flashowanie, debug, testy portu UART.

---

## Wymagania

- Linux + `bash`
- toolchain ARM:
  - `arm-none-eabi-gcc`
  - `arm-none-eabi-gdb`
- narzedzia ST-Link:
  - `st-flash`
  - `st-util`
  - `st-info`
- narzedzia pomocnicze:
  - `make`
  - `python3`
  - `minicom` (lub alternatywny terminal szeregowy)

Przyklad instalacji (Ubuntu):

```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi gdb-multiarch stlink-tools make python3 minicom
```

---

## Szybki start (najczesciej uzywany: `lwip_zero`)

1) Budowanie:

```bash
make lwip-zero
```

2) Flash:

```bash
make flash-lwip-zero
```

3) UART log (zwykle `ttyACM1`, ale sprawdz `dmesg`):

```bash
minicom -D /dev/ttyACM1 -b 115200
```

4) Debug GDB (w osobnych terminalach):

```bash
st-util
bash scripts/gdb_lwip_zero.sh
```

---

## Najwazniejsze targety `make`

- `make` - domyslnie buduje `bootloader`.
- `make lwip-zero` - budowa firmware `build/lwip_zero/lwip_zero.bin`.
- `make flash-lwip-zero` - flash `lwip_zero` pod `0x08000000`.
- `make lwip` / `make flash-lwip` - starszy profil LwIP.
- `make step1` / `make flash-step1` - etap 1 (LED/UART).
- `make step2` / `make flash-step2` - etap 2 (FreeRTOS + I2C).
- `make demo` / `make flash-demo` - profil demo.
- `make clean` - czyszczenie artefaktow build.

---

## Secure boot - stan i workflow

- Bootloader jest budowany z `bootloader/Makefile`.
- Aplikacja moze byc podpisana skryptem `scripts/sign_image.py`.
- Dostepny jest flow "signed app" z poziomu glownego `Makefile`.

Przykladowy przebieg:

```bash
make bootloader
make app
make signed-app
```

Uwaga: ustawienia produkcyjne (RDP/WRP/PCROP, blokada debug) nalezy wlaczac dopiero po pelnej walidacji procesu aktualizacji.

---

## Diagnostyka i typowe problemy

- ST-Link zajety:
  - Objaw: `st-flash` nie moze polaczyc sie z targetem.
  - Rozwiazanie: zatrzymaj `st-util` przed flashowaniem.

- Brak logow UART:
  - Sprawdz poprawny port (`/dev/ttyACM0` vs `/dev/ttyACM1`).
  - Ustaw `115200 8N1`.

- HardFault po starcie:
  - Uzyj `scripts/gdb_lwip_zero.sh` - skrypt wypisuje rejestry, CFSR/HFSR/BFAR i stos.

- Brak STM32CubeH7:
  - Niektore targety korzystaja z `CUBE_ROOT`.
  - Mozna nadpisac sciezke:

```bash
make demo CUBE_ROOT=/sciezka/do/STM32CubeH7
```

---

## Dalsza dokumentacja

- `docs/ARCHITECTURE.md` - mapa architektury projektu.
- `docs/LWIP_ANALYSIS_PL.md` - opis profilu `lwip_zero`.
- `docs/DEBUG_PL.md` - wskazowki debugowania.
- `docs/VERIFIED_BOOT_STM32_PL.md` - notatki o verified boot na STM32.

---

## Status projektu

Projekt ma charakter badawczo-edukacyjny. Kod i konfiguracje sa intensywnie iterowane pod konkretna plytke, dlatego przed uzyciem produkcyjnym konieczne sa:
- audyt bezpieczenstwa,
- testy dlugoczasowe,
- walidacja konfiguracji Option Bytes.
