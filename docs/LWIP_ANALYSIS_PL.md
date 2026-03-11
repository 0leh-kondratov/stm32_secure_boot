# Aplikacja do analizy kodu/lwip (LwIP + FreeRTOS)

## Znaleziono i naprawiono problemy

### 1. MPU: Obszar D2 dla RX_POOL nie został całkowicie pokryty (przyczyna HardFault)

**Esencja:** D2 SRAM zawiera:
- `0x30000000` - deskryptory Rx DMA
- `0x30000060` - Deskryptory Tx DMA
- `0x300000c0` — Pula RX_POOL (memp_memory_RX_POOL_base), rozmiar ~12–13 KB
- `0x30004000` - sterta LwIP (16 KB)

Określono region MPU 5: podstawa `0x30000000`, **rozmiar 1 KB** (do `0x300003FF`).
Bufory RX_POOL po pierwszym kilobajcie (`0x30000400`–`0x30003FFF`) trafiały do ​​strefy bez autoryzowanego dostępu (polityka MPU w tle) → dostęp do nich skutkował MemManage → HardFault.

**Poprawka:** Rozmiar regionu 5 zmieniono z 1 KB na **16 KB**, aby pokryć `0x30000000` - `0x30003FFF` (uchwyty + cały RX_POOL). Region 6 nadal określa dostęp do `0x30004000` (16 KB) dla sterty LwIP.

### 2. Stos zadań StartDefaultTask

**Podstawa:** Zadanie wywołuje `tcpip_init()`, `Netif_Config()` (w tym `netif_add` → `ethernetif_init` → `low_level_init`), `log_tcpip_params()` (bufor 128 bajtów na stosie). Stos 256×4 = 1024 bajtów może być kompleksowy lub prowadzić do przepełnienia.

**Korekta:** Rozmiar stosu zwiększono do **512x4 = 2048** bajtów.

---

## Zweryfikowane miejsca (bez zmian)

- **ETH_TxPacketConfig** w `ethernetif.c`: używany jako alias typu `ETH_TxPacketConfigTypeDef` (starsza wersja HAL), rozmiar jest prawidłowy.
- **Umieszczenie sekcji w D2** przez linker i `nm`: deskryptory i RX_POOL nie pokrywają się z obszarem `0x30004000` (lwip_ram_heap).
- **Kolejność inicjalizacji w głównym:** MPU → pamięć podręczna → HAL_Init → cykle zegara → log → BSP → jądro RTOS → utworzenie zadania - poprawne.
- **BSP_Config:** po włączeniu zegarów peryferyjnych `HAL_Init()` i `SystemClock_Config()`; Oczekiwana jest inicjalizacja diod LED i GPIO.

---

## Zalecenia dotyczące debugowania

1. Powtarzając HardFault w GDB, spójrz: `p/x g_hardfault_cfsr`, `p/x g_hardfault_bfar`, `p/x g_hardfault_pc`, `x/i g_hardfault_pc`.
2. Jeżeli BFAR wskazuje na D2 (0x30xxxxxx) - sprawdź ponownie MPU i rozmieszczenie sekcji w D2.
3. Jeśli nastąpi awaria w `StartDefaultTask` lub w LwIP, sprawdź rozmiar stosu tego zadania (obecnie 2 KB).
