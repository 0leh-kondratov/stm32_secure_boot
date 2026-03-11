# Architektura projektu STM32 Secure Boot

Projekt podzielony jest na dwie części: **Bootloader (Secure Boot)** i **Aplikacja (Ethernet + LCD + FreeRTOS)**.

---

## 1. Bootloader (Secure Boot)

- **Lokalizacja Flasha:** `0x08000000` - pierwsze 64 KB.
- **Zamiar:**
- Inicjalizacja HAL, zegarów, UART (logowanie).
- Weryfikacja podpisu aplikacji (ECDSA P-256 na obrazie SHA-256).
- Jeśli się powiedzie, przejdź do aplikacji („Jump_To_Application”).
- W przypadku błędu - sygnalizacja (LED) i zatrzymanie.
- **Weryfikacja podpisu:** SHA-256 według regionu aplikacji (HAL HASH lub bezpośredni dostęp do HASH), weryfikacja ECDSA (mbedTLS lub stub `USE_ECDSA_STUB`). Na płytach z PKA możliwe jest zastąpienie go przez `HAL_PKA_VerifySignature` (patrz komentarze w `bootloader/src/main.c`).
- **Kompilacja:** `utwórz bootloader` → `build/bootloader/bootloader.bin`. Oprogramowanie sprzętowe w `0x08000000`.

---

## 2. Application (Ethernet + LCD + FreeRTOS)

- **Lokalizacja Flash:** obraz z nagłówkiem z `0x08010000` (lub jeśli to konieczne z `0x08020000` - zobacz `memory_map.h` i skrypt podpisu).
- **Zamiar:**
- Sieć: LwIP (Ethernet) + DHCP, w razie potrzeby HTTPS (mbedTLS + LwIP altcp_tls).
- Wyświetlacz: I2C LCD 1602 (adres 0x27), zadanie `lcd_monitor_task` aktualizuje IP i status.
- System operacyjny: FreeRTOS + CMSIS-RTOS V2.
- **Przed uruchomieniem harmonogramu:** opcjonalna kontrola integralności (SHA-256 według obrazu oprogramowania sprzętowego, na przykład według regionu `0x08020000` lub bieżącego obrazu) poprzez HAL HASH.
- **Kompilacja:** `make lwip` (lub oddzielna aplikacja docelowa z pełną listą modułów) → obraz oprogramowania sprzętowego po nagłówku (podpisany - `make Signed-app`).

---

## 3. Udostępnianie pamięci (przykład)

| Region | Adres | Rozmiar | Opis |
|----------------|--------------|----------|-----------------------------|
| Bootloader     | 0x08000000   | 64 KB    | Secure Boot                  |
| Nagłówek aplikacji+kod| 0x08010000 | do 2MB | Tytuł + aplikacja |
| SRAM (DMA ETH) | 0x30040000 | przez MPU | Deskryptory LwIP (SRAM3) |
| Heap FreeRTOS  | —            | ≥ 128 KB | configTOTAL_HEAP_SIZE       |

---

## 4. Przebieg ładowania

1. Zresetuj → uruchomiony program ładujący.
2. Bootloader: weryfikacja podpisu obrazu aplikacji.
3. Jeśli się powiedzie: `Jump_To_Application(entry_point)` (VTOR, MSP, przejdź do Reset_Handler aplikacji).
4. Aplikacja: init HAL, zegar, opcjonalna integralność SHA-256, BSP, inicjalizacja jądra FreeRTOS, tworzenie zadań (LwIP, LCD, HTTPS itp.), `osKernelStart()`.

---

## 5. Lista plików Makefile

Zobacz **docs/MAKEFILE_FILES_LIST.md** - ścieżki do Core/HAL (Ethernet, I2C, HASH, PKA jeśli są dostępne), LwIP, FreeRTOS, mbedTLS.
