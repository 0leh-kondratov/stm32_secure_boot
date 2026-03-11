# Harmonogram testów (od prostych do złożonych)

Testowanie krok po kroku sprzętu i stosu przed pełną integracją.

---

## Etap 1: „Żelazo na żywo” (LCD + skaner I2C)

**Cel:** Upewnij się, że procesor widzi wyświetlacz. Na H7 ​​magistrala I2C często nie uruchamia się z powodu nieprawidłowych czasów lub braku podciągnięcia.

**Zadanie:** Przeskanuj magistralę I2C i wyślij adres (0x27 lub 0x3F) do UART.

**Co sprawdzamy:** Przewody łączące, zasilanie 5 V, czasy I2C.

**Aplikacja testowa:** Pętla w `main` bez FreeRTOS, `HAL_I2C_IsDeviceReady`.

| Krok | Testuj | Pliki | Wynik |
|-----|-------------|---------------------------------|------------------------|
| 1 | Skaner I2C | `stm32h7xx_hal_i2c.c` | Adres **0x27** w konsoli |

**Montaż i oprogramowanie sprzętowe:**
```bash
make test-stage1
st-flash write build/stage1/stage1.bin 0x08000000
```
Otwarty zacisk 115200 8N1 (ST-Link VCP). Po zresetowaniu w konsoli powinny pojawić się linie typu `[FOUND] 0x27`.

---

## Etap 2: „Rytm systemu” (FreeRTOS + LCD)

**Cel:** Sieć (LwIP) wymaga stabilnego harmonogramu. Jeśli FreeRTOS nie jest poprawnie skonfigurowany, stos będzie się zacinał.

**Zadanie:** Jedno zadanie zwiększa licznik i wyświetla go na wyświetlaczu LCD raz na sekundę.

**Co sprawdzamy:** Konfiguracja Systick (na H7 jest to krytyczne), działanie wyświetlania wewnątrz strumienia.

| Krok | Testuj | Pliki | Wynik |
|-----|----------------|--------------------------|-----------------------------|
| 2 | Witaj świecie LCD| `lcd_1602_i2c.c` + FreeRTOS | **Napis „Wallet Init”** na ekranie, licznik co 1 s |

**Montaż i oprogramowanie sprzętowe:**
```bash
make test-stage2
st-flash write build/stage2/stage2.bin 0x08000000
```
Na wyświetlaczu LCD: pierwsza linia to „Count: N”, druga to „Wallet Init”, aktualizowana raz na sekundę.

---

## Etap 3: „Fizyka sieci” (test pingu)

**Cel:** Najtrudniejszą częścią H7 są deskryptory MPU i Ethernet w SRAM3.

**Zadanie:** Uruchom LwIP w minimalnej konfiguracji (ICMP/Ping).

**Co sprawdzamy:** Konfigurowanie MPU (Memory Protection Unit), dostępu DMA do pamięci. Jeśli polecenie ping działa, oznacza to, że „sprzętowa” część sieci i sterownik działają.

| Krok | Testuj | Pliki | Wynik |
|-----|-----------|--------------------------|---------------------------|
| 3 | Ping LwIP | `ethernetif.c`, `lwipopts.h` | Stabilny **Ping &lt; 1 ms** |

**Jak sprawdzić:**
```bash
make lwip
st-flash write build/lwip/lwip.bin 0x08000000
# Podłącz Ethernet, poczekaj na DHCP lub ustaw statyczny adres IP w lwipopts.h/app.
ping 192.168.1.XX
```
Z Ubuntu: `ping -c 5 192.168.x.x`. Pomyślna wymiana i niskie opóźnienia oznaczają, że MPU i DMA są poprawnie skonfigurowane.

---

## Etap 4: „Silnik kryptograficzny” (weryfikacja podpisu)

**Cel:** Sprawdź, czy weryfikacja podpisu i format klucza/podpisu są zgodne z tym, co tworzy skrypt Pythona.

**Zadanie:** Weź przygotowany wcześniej skrót i podpis i zweryfikuj je. Na tablicach z **PKA** - `HAL_PKA_VerifySignature`. Na **STM32H743** nie ma PKA - używana jest weryfikacja oprogramowania (mbedTLS ECDSA lub kod pośredniczący w bootloaderze).

**Co sprawdzamy:** Zgodność formatu (podpis ze skryptu = czego oczekuje STM32).

| Krok | Testuj | Pliki | Wynik |
|-----|---------|-------------------------|----------------------------|
| 4 | Test PKA/ECDSA | `stm32h7xx_hal_pka.c` (jeśli jest dostępny) lub bootloader + mbedTLS | Komunikat **„Auth OK”** (UART lub LCD) |

**Jak sprawdzić:** Uruchom bootloader z podpisanym obrazem aplikacji. UART powinien mieć `[boot] Signature OK` i przejść do aplikacji. W razie potrzeby wyświetl na wyświetlaczu LCD w aplikacji po pomyślnej weryfikacji „Auth OK” (lub w bootloaderze w przypadku dodania tam LCD).

---

## Etap 5: Serwer HTTP

**Cel:** Otwórz stronę internetową w przeglądarce.

**Zadanie:** Skonfiguruj serwer HTTP(S) (Netconn API, jeśli to konieczne - mbedTLS dla HTTPS).

| Krok | Testuj | Pliki | Wynik |
|-----|--------------|----------------------------|------------------------------------|
| 5 | Serwer HTTP | `httpsserver_netconn.c` itp. | Otwieranie strony w przeglądarce poprzez IP |

**Jak sprawdzić:** Po zbudowaniu aplikacji LwIP z serwerem HTTP otwórz w przeglądarce `http://192.168.x.x/`. Dla HTTPS - port 443 i mbedTLS (patrz `docs/INTEGRATION_FINAL.md`).

---

## Tabela przestawna

| Krok | Testuj | Pliki do wykorzystania | Wynik |
|-----|-------------|------------------------------|------------------------------|
| 1 | Skaner I2C | `stm32h7xx_hal_i2c.c` | Adres **0x27** w konsoli |
| 2 | Witaj świecie LCD | `lcd_1602_i2c.c` + FreeRTOS | Na ekranie napis **„Wallet Init”** |
| 3 | Ping LwIP | `ethernetif.c`, `lwipopts.h` | Stabilny **Ping &lt; 1 ms** |
| 4 | Test PKA/ECDSA | `stm32h7xx_hal_pka.c` lub program ładujący | **Komunikat „Auth OK”** na wyświetlaczu LCD/UART |
| 5 | Serwer HTTP | `httpsserver_netconn.c` | Otwieranie strony w przeglądarce |

---

## Cele Makefile w katalogu głównym projektu

- `make test-stage1` - Kompilacja etapu 1 (skaner I2C).
- `make test-stage2` - Kompilacja etapu 2 (FreeRTOS + licznik LCD).
- `make lwip` — budowanie aplikacji za pomocą LwIP (Etap 3: Ping).
- Bootloader i podpis - Etap 4 (Auth).
- Serwer HTTP/HTTPS - Etap 5 (patrz aktualny cel LwIP i dodanie HTTP).

Oprogramowanie sprzętowe dla testów etapu 1/2 to zawsze **0x08000000** (bez programu ładującego). Aby sprawdzić za pomocą bootloadera: bootloader pod adresem 0x08000000, podpisana aplikacja pod adresem 0x08010000.
