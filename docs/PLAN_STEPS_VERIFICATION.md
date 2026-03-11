# Plan wdrożenia i testów krok po kroku (Kroki 1–5 + LwIP/HTTP/DHCP/TLS)

Plan: Bazowy sprzęt → FreeRTOS → LwIP+MPU → Bezpieczny rozruch (SHA-256) → Netconn (port 7) → opcjonalny serwer HTTP + DHCP + TLS.

---

## Podsumowanie: obrazy demonstracyjne i polecenia

| Krok | Cel | Montaż | Oprogramowanie sprzętowe | Sprawdź |
|------|------|--------|----------|--------------|
| **1** | Skaner I2C + LCD | `zrób krok 1` lub `zrób etap testowy 1` | `Utwórz etap flash1` / `Utwórz etap flash1` | UART: „Urządzenie o 0x27”; LCD: „Gotowy” |
| **2** | FreeRTOS + DisplayTask | `zrób krok 2` lub `zrób etap testowy 2` | `Utwórz etap flash2` / `Utwórz etap flash2` | LCD: licznik raz na 1 s; SysTick jest stabilny |
| **3** | LwIP + Ethernet + MPU | `make lwip` (lub krok 3) | `zrób flash-lwip` | `ping <IP>` z Ubuntu |
| **4** | Program ładujący + SHA-256 | `utwórz bootloader` + aplikacja | `./scripts/flash.sh` lub ręcznie | UART: Log SHA-256, porównanie ze standardem |
| **5** | Port serwera Netconn 7 + echo LCD | krok 5 (nowy cel) | flash-step5 | `telnet <IP> 7` → tekst na wyświetlaczu LCD |
| **6** | HTTP + DHCP + TLS (opcjonalnie) | krok6/lwip-http | flash-step6 | HTTPS w przeglądarce, DHCP IP |

Płytka: **NUCLEO-H743ZI2** (MB1364). ROZRUCH0=0. UART: USART3, 115200 8N1.

---

## Step 1: Hardware Baseline (I2C + LCD)

**Cel:** I2C1 (PB8/PB9), skaner, LCD 1602 (PCF8574 @ 0x27), brak RTOS.

**Co zawiera projekt:**
- **aplikacja/krok 1** — I2C + linia bazowa LCD.
- **test/stage1_i2c_scanner** — skaner I2C, wyjście na UART.

**Zalecenie:** Wybierz jeden obraz dla kroku 1 (na przykład krok 1 z wyświetlaczem LCD „Ready” + UART), aby się nie duplikować.

| Akcja | Zespół |
|---------|---------|
| Montaż | `zrób krok 1` **lub** `zrób etap testowy 1` |
| Oprogramowanie sprzętowe | `utwórz etap flash1` **lub** `utwórz etap flash1` |
| Sprawdź | 1) UART: ciąg znaków typu „Device Found at 0x27” (lub lista adresów). 2) LCD: „Gotowy” (jeśli krok 1 zawiera LCD_Print). |

**Kryterium sukcesu:** Adres 0x27 jest widoczny w UART; na wyświetlaczu LCD - „Gotowy” (dla kroku 1).

---

## Step 2: OS Heartbeat (FreeRTOS + DisplayTask)

**Cel:** FreeRTOS, jedno DisplayTask, licznik na wyświetlaczu LCD raz na 1 s; HAL_Delay i vTaskDelay są spójne.

**Co zawiera projekt:**
- **app/step2** - FreeRTOS + DisplayTask, licznik na LCD.
- **test/stage2_freertos_lcd** — FreeRTOS + LCD, „Wallet Init”, licznik.

| Akcja | Zespół |
|---------|---------|
| Montaż | `zrób krok 2` **lub** `zrób etap testowy 2` |
| Oprogramowanie sprzętowe | `Utwórz etap flash2` **lub** `Utwórz etap flash2` |
| Sprawdź | LCD: licznik zwiększa się co sekundę; UART, jeśli jest obecny, jest stabilnym wyjściem. SysTick nie pływa (nie ma podwójnej init SysTick). |

**Kryterium sukcesu:** Licznik na wyświetlaczu LCD raz na 1 s; system nie zawiesza się.

---

## Step 3: Network Physical Layer (LwIP + MPU)

**Cel:** Ethernet MAC, MPU (obsługi w SRAM3 0x30040000, bez pamięci podręcznej), ping.

**Co zawiera projekt:**
- **app/lwip** — LwIP + FreeRTOS, echo TCP (lub minimalny stos); linker `lwip_nukleo_h743zi.ld` (sprawdź sekcje w DMA/MPU).

**Zadania:** Upewnij się, że linker ma region dla deskryptorów DMA (SRAM3); w kodzie - inicjalizacja MPU dla tego obszaru (Non-Cacheable). Jeśli to konieczne, użyj Cube **LwIP_HTTP_Server_Netconn_RTOS** (ethernetif, MPU_Config) jako podstawy.

| Akcja | Zespół |
|---------|---------|
| Montaż | `zrób lwip` |
| Oprogramowanie sprzętowe | `zrób flash-lwip` |
| Sprawdź | Płyta znajduje się w tej samej podsieci co komputer. Odpowiedzią jest `ping <IP>` z Ubuntu. IP - statyczny (ustawiany w lwipopts.h/config) lub DHCP (patrz krok 6). |

**Kryteria sukcesu:** Pomyślny ping z hosta na tablicę.

---

## Step 4: Secure Bootloader (SHA-256 Integrity)

**Cel:** Weryfikacja integralności obrazu aplikacji za pomocą HAL HASH (SHA-256); zaloguj się UART, porównanie ze standardem.

**Co zawiera projekt:**
- **bootloader/** - Bezpieczny rozruch (ECDSA), jeśli to konieczne, dodaj/włącz Verify_Integrity() na HAL HASH.
- Oddzielny obraz tylko do weryfikacji SHA-256 można umieścić w kroku 4 (program ładujący + aplikacja pośrednicząca) w celu izolowanej weryfikacji.

| Akcja | Zespół |
|---------|---------|
| Montaż | `utwórz program ładujący` i aplikację (na przykład `utwórz aplikację` + podpis lub obraz testowy). |
| Oprogramowanie sprzętowe | `./scripts/flash.sh` lub ręcznie: bootloader @ 0x08000000, (podpisana) aplikacja @ 0x08010000. |
| Sprawdź | UART: log obliczonego SHA-256 obszaru zastosowania; porównanie z wartością zakodowaną na stałe; jeśli jest zgodność, przejdź do aplikacji (LED/logi). |

**Kryterium sukcesu:** W logu bootloadera – obliczony SHA-256 i wynik porównania; jeśli się powiedzie, uruchom aplikację.

---

## Krok 5: Interaktywny TCP (Netconn, port 7, echo na wyświetlaczu LCD)

**Cel:** Zadanie Netconn: odsłuchaj port 7, odbierz linię, wyświetl ją na wyświetlaczu LCD (echo na ekranie).

**Realizacja:** Nowy cel w projekcie (np. **app/step5** lub rozszerzenie **app/lwip**): Serwer LwIP Netconn na porcie 7, zadanie FreeRTOS, przy odbiorze danych - zapisanie linii do bufora i wyświetlenie jej na wyświetlaczu LCD poprzez istniejące API LCD.

| Akcja | Zespół |
|---------|---------|
| Montaż | `zrób krok 5` (po dodaniu celu) |
| Oprogramowanie sprzętowe | `wykonaj flash-krok 5` |
| Sprawdź | `telnet <IP> 7`, wprowadź linię - ta sama linia pojawi się na wyświetlaczu LCD. |

**Kryterium sukcesu:** Telnet jest podłączony, wprowadzony tekst jest wyświetlany na wyświetlaczu LCD.

---

## Krok 6 (opcjonalnie): Serwer HTTP LwIP + DHCP + TLS

**Pytanie:** Czy można dodać LwIP_HTTP_Server_Netconn_RTOS + DHCP + TLS?

**Tak, krok po kroku:**

1. **HTTP + Netconn RTOS** – na przykładzie z Cube:  
   `STM32CubeH7/Projects/NUCLEO-H743ZI/Applications/LwIP/LwIP_HTTP_Server_Netconn_RTOS`, przenieś do projektu (lub skopiuj zadanie ethernetif, http, netconn) i zbuduj jako osobny obraz (na przykład **step6** lub **lwip-http**).
2. **DHCP** - w lwipopts.h: `LWIP_DHCP 1`; W kodzie inicjującym netif wywołaj `dhcp_start()`. Sprawdź: płyta otrzymuje adres IP z routera; Otrzymany adres IP możesz wyświetlić w logu/UART lub na wyświetlaczu LCD.
3. **TLS (HTTPS)** - użyj LwIP **altcp_tls** (mbedTLS pod maską): podnieś serwer HTTPS do 443, certyfikat/klucz na urządzeniu. Wymaga większej integracji pamięci RAM/Flash i mbedTLS (z Cube lub osobno).

**Kolejność realizacji:** najpierw HTTP + DHCP (obrazek kroku 6), sprawdzanie w przeglądarce i przez DHCP; następnie dodaj mbedTLS i altcp_tls dla HTTPS.

| Akcja | Zespół (po wdrożeniu) |
|-------------|----------------------------|
| Montaż | `zrób krok 6` lub `zrób lwip-http` |
| Oprogramowanie sprzętowe | `wykonaj krok flash6` |
| Sprawdź | DHCP: Płyta otrzymuje adres IP. Przeglądarka: http://<IP> - strona. HTTPS: po integracji z TLS - https://<IP>. |

---

## Zalecana kolejność testowania

1. **Krok 1** - flashuj krok 1 (lub etap 1), upewnij się, że UART + LCD.
2. **Krok 2** — miga krok 2 (lub etap 2), sprawdza licznik co 1 s.
3. **Krok 3** - zflashuj lwip, skonfiguruj sieć, sprawdź ping.
4. **Krok 4** - wgraj bootloader + aplikację, sprawdź log SHA-256 i przejdź do aplikacji.
5. **Krok 5** - zaimplementuj port Netconn 7 + echo LCD, flash, sprawdź telnet i LCD.
6. **Krok 6** – w razie potrzeby: Serwer HTTP + DHCP, następnie TLS.

Każdy krok to osobny obraz demonstracyjny i jasne kryterium weryfikacji (UART/LCD/ping/telnet/HTTP), aby zmiany nie przerywały poprzednich etapów.

---

## Ścieżki testowe

Obydwa etapy zbudowane są z katalogu **test/**:
- **etap-testowy1** → `test/etap1_i2c_scanner`
- **etap testowy2** → `test/etap2_freertos_lcd`