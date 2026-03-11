# Ostateczna integracja (Ethernet + LCD + FreeRTOS + Bezpieczny rozruch)

Krótka lista kontrolna elementów wniosku i tego, co zostało zrobione.

---

## 1. Architektura: Bootloader i aplikacja

- **Bootloader:** `utwórz bootloader` → `build/bootloader/bootloader.bin` (0x08000000). Weryfikacja podpisu (SHA-256 + ECDSA), jeśli przebiegła pomyślnie, przejdź do aplikacji. Zobacz `docs/ARCHITECTURE.md`.
- **Aplikacja:** LwIP + FreeRTOS + LCD + (opcjonalnie HTTPS). Kompilacja: `make lwip` → `build/lwip/lwip.bin`; Obraz jest podpisany i flashowany kodem 0x08010000.

---

## 2. Lista plików Makefile

- **HAL:** `stm32h7xx_hal_eth.c`, `stm32h7xx_hal_i2c.c`, `stm32h7xx_hal_hash.c`.  
  `stm32h7xx_hal_pka.c` - tylko dla chipów z PKA (nie na H743); zobacz `docs/MAKEFILE_FILES_LIST.md`.
- **Middleware:** LwIP, FreeRTOS, mbedTLS (dla HTTPS) - ścieżki w `docs/MAKEFILE_FILES_LIST.md` i `app/lwip/Makefile`.

---

## 3. I2C LCD 1602 w FreeRTOS

- **Sterownik:** `common/lcd_1602_i2c.c`, `common/lcd_1602_i2c.h` (adres 0x27, PCF8574).
- **Zadanie:** `lcd_monitor_task` (nazwa w kodzie: `LcdMonitorTask`):
  - okres 500 ms;
  - linia 1: aktualne IP (z LwIP);
  - linia 2: `"Bezpieczny rozruch OK"`;
  - dostęp do I2C1 jest chroniony przez **Mutex** (`I2C1_MutexHandle`).
- Podczas uruchamiania na wyświetlaczu LCD pojawia się komunikat „Booting Secure OS” / „”…”, a następnie DHCP – IP.

---

## 4. Bezpieczny rozruch (sprawdź przed skokiem)

- W `bootloader/src/main.c`: funkcja `verify_signature_and_ready_to_jump()` wywołuje `verify_signature()` (SHA-256 na obraz aplikacji + ECDSA). W przypadku powodzenia - `jump_to_application(hdr->entry_point)`, w przypadku błędu - `signal_verification_failure()` (LED, stop).
- Komentarze w kodzie: na płytach z PKA można zastąpić `HAL_PKA_VerifySignature(&hpka, &sig_params)`.
- Wyjście na wyświetlacz LCD z bootloadera nie jest zaimplementowane (brak I2C w bootloaderze); Jeśli to konieczne, możesz dodać minimalny sterownik LCD do programu ładującego.

---

## 5. Sieć: HTTPS (LwIP + mbedTLS)

- W **lwipopts.h** zawarte: `LWIP_ALTCP 1`, `LWIP_ALTCP_TLS 1`. Do pełnego protokołu HTTPS potrzebujesz:
  - dodaj mbedTLS do zestawu (źródła i `mbedtls_config.h` z Cube);
  - włącz port LwIP dla mbedTLS (`LWIP_ALTCP_TLS_MBEDTLS`, pliki z `app/lwip/apps/altcp_tls/`);
  - zaimplementuj serwer na porcie 443 w oparciu o Netconn API na wierzchu `altcp_tls` (podobnie do przykładu LwIP_HTTP_Server_Netconn_RTOS w `Projects/NUCLEO-H743ZI/Applications/LwIP/`).
- Echo TCP (port 7) jest teraz zmontowane; Serwer HTTPS na 443 to kolejny krok przy łączeniu mbedTLS.

---

## 6. Integracja: pamięć, MPU, integralność, sterta

- **MPU:** W `main.c` `MPU_Config()` nosi nazwę: region dla ETH DMA (0x30000000, 1 KB), region dla buforów LwIP (0x30004000, 16 KB). Używając deskryptorów w **SRAM3 (0x30040000)** musisz dodać/dostosować region MPU dla tego adresu (patrz linker i ethernetif).
- **SHA-256 przed harmonogramem:** Dodano komentarz i fragment pośredni do `main()` wywołujący kontrolę integralności obrazu pod adresem 0x08020000 (lub 0x08010000). Implementacja - poprzez HAL HASH (`stm32h7xx_hal_hash.c`) w aplikacji i porównanie z oczekiwanym skrótem; jeśli to konieczne, odkomentuj i zaimplementuj `firmware_integrity_check()`.
- **Sterta FreeRTOS:** `FreeRTOSConfig.h` jest ustawiona na **configTOTAL_HEAP_SIZE = 128 KB** dla zadań LwIP + mbedTLS +.
- **ethernetif:** Używane w przykładzie H743ZI (lub kopii lokalnej), uchwyty i bufory znajdują się w sekcjach określonych w skrypcie linkera (`lwip_nukleo_h743zi.ld`).

---

## Montaż

- Bootloader: `utwórz bootloader`
- Aplikacja (LwIP + LCD + FreeRTOS): `make lwip`
- Obraz podpisanej aplikacji: `make Signed-app` (po zbudowaniu app/lwip i wstawieniu prawidłowego pliku app.bin do skryptu podpisu)

Jeśli to konieczne, przenieś aplikację do **0x08020000**: zmień ORIGIN w skrypcie linkera aplikacji oraz adres w skrypcie podpisu i w `memory_map.h`.