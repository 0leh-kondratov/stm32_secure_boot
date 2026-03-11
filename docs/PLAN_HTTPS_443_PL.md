# Plan: serwer HTTPS na porcie 443 (Netconn + mbedTLS) i 128 KB pamięci

Cel: serwer odpowiadający na żądania GET przez HTTPS (port 443) przy użyciu mbedTLS; Sterta FreeRTOS o wielkości co najmniej 128 KB dla LwIP i mbedTLS.

---

## 1. Pamięć (configTOTAL_HEAP_SIZE ≥ 128 KB)

**Cel:** Zapewnienie co najmniej 128 KB sterty FreeRTOS dla LwIP, mbedTLS i zadań.

**Pliki i edycje:**

| Plik | Aktualna wartość | Akcja |
|------|------------------|----------|
| `FreeRTOS/Źródło/include/FreeRTOSConfig_lwip.h` | `40 * 1024` (40 KB) | Zamień na `(128 * 1024)` |
| `app/lwip_zero/Inc/FreeRTOSConfig.h` (jeśli został użyty do zbudowania LwIP) | `40 * 1024` | To samo: `(128 * 1024)` |

**Sprawdź:** po zbudowaniu obrazu LwIP upewnij się, że linker nie narzeka na przepełnienie RAM (sekcja sterty w RAM). Jeśli to konieczne, zwiększ rozmiar sterty/stosu w linkerze lub pozostaw 128 KB w dostępnej pamięci RAM (H743 ma wystarczająco dużo).

---

## 2. HTTPS: serwer na 443 z mbedTLS

**Pomysł:** Serwer w stylu Netconn (akceptuj → czytaj → analizuj GET → wysyłaj odpowiedź), ale transport odbywa się w trybie TLS na porcie 443 przez LwIP **altcp_tls** (pod maską mbedTLS).

**Ważne:** Interfejs API LwIP Netconn nie obsługuje protokołu TLS po wyjęciu z pudełka. TLS odbywa się poprzez **altcp** (TCP warstwy aplikacji): wywołujemy słuchacza **altcp_pcb** z typem **altcp_tls** na porcie 443, poprzez akceptację otrzymujemy połączenie TLS, następnie czytamy/zapisujemy przez altcp_* - logika jest taka sama jak HTTP Netconn (parsowanie GET, wysyłanie odpowiedzi).

---

## 3. Plan wykonania krok po kroku

### Krok 1. Sterta 128 KB

- W `FreeRTOSConfig_lwip.h` i jeśli to konieczne, w `app/lwip_zero/Inc/FreeRTOSConfig.h` ustaw:
  - `configTOTAL_HEAP_SIZE ((size_t)(128 * 1024))`
- Odbuduj obraz LwIP, sprawdź linker i działanie (ping, istniejący HTTP, jeśli jest dostępny).

### Krok 2. Połącz mbedTLS

- **Źródło mbedTLS:** STM32CubeH7
`Middlewares/Third_Party/mbedtls/` (lub odpowiednik z innego repozytorium projektu).
- Dodaj do zestawu:
- uwzględnij ścieżki: `Middlewares/Third_Party/mbedtls/include`, `Middlewares/Third_Party/mbedtls/library`;
- wymagane `mbedtls_*.c` dla minimalnej konfiguracji serwera TLS (patrz `mbedtls_config.h` w Cube; zwykle: ssl, ssl_tls, x509, pk, pk_wrap, cipher, md, itp.).
- Upewnij się, że projekt ma jeden wspólny plik `mbedtls_config.h` (można to zrobić z poziomu Cube), bez zbędnych funkcji oszczędzania Flash/RAM.

### Krok 3. LwIP: altcp + altcp_tls_mbedtls

- W **lwipopts.h** (lub w opcjach LwIP, których używa Twój obraz):
  - `LWIP_ALTCP 1`
  - `LWIP_ALTCP_TLS 1`
  - `LWIP_ALTCP_TLS_MBEDTLS 1`
- Dodaj źródła LwIP do zestawu:
  - `altcp.c`
- z `lwip/apps/altcp_tls/`: `altcp_tls_mbedtls.c`, w razie potrzeby `altcp_tls_mbedtls_mem.c` (jeśli używasz własnego alokatora w mbedTLS).
- Ścieżka do mbedTLS musi być dostępna podczas kompilowania tych plików.

### Krok 4. Certyfikat i klucz serwera

- Do testu wystarczy certyfikat i klucz z podpisem własnym (PEM lub DER, w zależności od tego, co akceptuje altcp_tls_mbedtls).
- Opcje:
- Wygeneruj na komputerze PC (OpenSSL) i osadź w oprogramowaniu w postaci tablic C (lub w osobnym pliku .c/.h).
- Weź gotowy certyfikat/klucz testowy z przykładów mbedTLS/Cube i zintegruj go z projektem.
- W kodzie inicjującym serwer TLS prześlij te dane (certyfikat + klucz prywatny) do mbedTLS/LwIP.

### Krok 5. Zadanie serwera HTTPS (port 443)

- W oddzielnym module (na przykład `https_server_netconn.c` lub `https_server_altcp.c`):
- Uruchom w kontekście wątku TCPIP (lub oddzielnego zadania FreeRTOS wywołującego tylko LwIP-API z pojedynczego wątku).
- Utwórz słuchacza **altcp_pcb** dla portu 443 poprzez **altcp_tls** (typ TLS, konfiguracja mbedTLS: certyfikat, klucz, opcje weryfikacji).
- W pętli: **altcp_accept** → nowa płytka PCB (już TLS); odczytaj dane poprzez **altcp_recv** (w trybie wywołania zwrotnego lub w trybie blokowania, jeśli API na to pozwala), przeanalizuj pierwszą linię żądania i sprawdź, czy jest to `GET /` lub `GET /...`.
- Wygeneruj minimalną odpowiedź HTTP, na przykład:
    - `HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n`
- treść: prosta strona HTML lub tekst „OK”.
- Wyślij przez **altcp_write**, zamknij połączenie (**altcp_close**), zwolnij płytkę drukowaną.
- Priorytet zadania jest podobny do istniejącego protokołu HTTP Netconn (na przykład normalny/wyższy niż normalny), rozmiar stosu jest z marginesem dla mbedTLS (na przykład 2–4 KB).

### Krok 6. Inicjalizacja w main/LwIP

- Po inicjalizacji LwIP i netif (oraz w razie potrzeby DHCP) wywołaj funkcję inicjalizacji serwera HTTPS (tworząc zadanie/wątek nasłuchujący 443 poprzez altcp_tls).
- Upewnij się, że port 443 nie jest zajęty przez inny kod.

### Krok 7. Montaż i testowanie

- Kompilacja: Docelowy plik Makefile/IDE powinien zawierać mbedTLS, altcp, altcp_tls_mbedtls, nowy moduł serwera HTTPS i zaktualizowany FreeRTOSConfig (128 KB).
- Badanie:
- Opłata sieciowa (statyczny adres IP lub DHCP).
- W przeglądarce: `https://<IP_board>` (lub `https://<IP>:443`). Oczekuje się ostrzeżenia o certyfikacie z podpisem własnym - zaakceptuj wyjątek, po czym powinna otworzyć się strona odpowiedzi GET (taka jak „OK” lub minimalna strona HTML).

---

## 4. Zagrożenia i uproszczenia

- **RAM/Flash:** mbedTLS i TLS zwiększają zużycie. Sterta 128 KB - minimalna; jeśli brakuje, możesz zwiększyć go do 160–192 KB, jeśli linker i pamięć RAM na to pozwalają.
- **Kompatybilność API:** w wersji LwIP z Cube/twojego drzewa dokładne nazwy i podpisy altcp_tls (tworzenie słuchacza, wiązanie certyfikatu) mogą się różnić - spójrz na nagłówki `lwip/apps/altcp_tls.h` i `altcp_tls_mbedtls*.h` w twoim klonie LwIP.
- **Uproszczenie na pierwszym etapie:** możesz nasłuchiwać TLS tylko na 443 i odpowiadać jedną stałą linią na dowolny GET, bez analizowania adresów URL i bez systemu plików.

---

## 5. Krótka lista kontrolna

1. [ ] W FreeRTOSConfig (LwIP) ustaw `configTOTAL_HEAP_SIZE` ≥ 128 KB.
2. [ ] Podłącz mbedTLS do zestawu (dołącz + wymagane .c).
3. [ ] W lwipopts włącz LWIP_ALTCP, LWIP_ALTCP_TLS, LWIP_ALTCP_TLS_MBEDTLS; dodaj altcp.c i altcp_tls_mbedtls.c.
4. [ ] Przygotuj certyfikat i klucz serwera (np. z podpisem własnym), osadź go w projekcie.
5. [ ] Zaimplementuj zadanie HTTPS: altcp_tls słuchaj 443, akceptuj → czytaj → parsuj GET → wyślij odpowiedź HTTP → zamknij.
6. [ ] Powoduje inicjalizację serwera HTTPS po uruchomieniu LwIP.
7. [ ] Zbuduj obraz, flashuj go, sprawdź w przeglądarce poprzez https://<IP>.

Po wykonaniu planu serwer przetworzy GET na porcie 443 przez TLS, a sterta FreeRTOS będzie wystarczająca dla LwIP i mbedTLS.
