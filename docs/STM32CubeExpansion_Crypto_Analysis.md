# Analiza pakietu STM32CubeExpansion_Crypto V4.6.0

Pakiet znajduje się w ścieżce: `/data/projects/STM32CubeExpansion_Crypto_V4.6.0` (żądanie zawiera ścieżkę z literówką `...V4.6.0./` - dodatkowa kropka na końcu).

---

## 1. Cel

**X-CUBE-CRYPTOLIB** - firmware kryptograficzny dla wszystkich serii STM32 firmy STMicroelectronics:

- Algorytmy testowane są w ramach **NIST ACVP** (certyfikacja).
- Obsługuje **Cortex-M0/M0+, M3, M4, M7, M33, M55**.
- Kompilacja dla **GCC, kompilatora ARM, IAR EWARM**.

---

## 2. Struktura katalogów

```
STM32CubeExpansion_Crypto_V4.6.0/
├── Release_Notes.html # Historia zmian pakietu
├── Package_license.html
├── sbom_cdx.json           # SBOM (CycloneDX v1.5)
├── Sterowniki/ # HAL, CMSIS, BSP według serii (G0, G4, H5, H7, H7RS, L0–L5, N6, U0/U3/U5, WB, WBA, WL itp.)
├── Middlewares/
│   └── ST/
│ ├── STM32_Cryptographic/ # Główna biblioteka kryptograficzna (v4.0.5)
│ ├── STM32_ExtMem_Loader/ # Program ładujący do pamięci zewnętrznej
│ └── STM32_ExtMem_Manager/ # Menedżer pamięci zewnętrznej (NOR, PSRAM, SD itp.)
└── Projekty/ # Przykłady na tablicach (NUCLEO-*, P-NUCLEO-WB55)
```

---

## 3. Biblioteka kryptograficzna (STM32_Cryptographic)

### 3.1 Wdrożenie

- **Biblioteki** - prekompilowane `.a` w `Middlewares/ST/STM32_Cryptographic/lib/`:
  - `libSTM32Cryptographic_CM0_CM0PLUS.a`
  - `libSTM32Cryptographic_CM3.a`
  - `libSTM32Cryptographic_CM4.a`
- `libSTM32Cryptographic_CM7.a` ← **dla STM32H743 (Cortex-M7)**
  - `libSTM32Cryptographic_CM33.a`
- `libSTM32Cryptographic_CM55.a` (w tym dla STM32N6)
- W pakiecie nie ma kodów źródłowych algorytmów - jedynie nagłówki i **legacy_v3** (opakowania starego API).

### 3.2 API (CMOX — Cortex-M Optimized)

Pojedynczy punkt wejścia dla nagłówków: `#include "cmox_crypto.h"` (pobiera wszystkie moduły).

| Moduł | Nagłówki/algorytmy |
|----------|------------------------|
| **Cipher** | AES (CBC, CCM, CFB, CTR, ECB, GCM, OFB, XTS, Keywrap), SM4, ChaCha20-Poly1305 |
| **Hash** | SHA-1, SHA-224/256/384/512, SHA-3, SM3, SHAKE |
| **MAC**  | CMAC, HMAC, KMAC |
| **RSA**  | PKCS#1 v1.5, PKCS#1 v2.2 (Encrypt/Decrypt, Sign/Verify) |
| **ECC**  | ECDSA, EdDSA, SM2, ECDH |
| **DRBG** | CTR_DRBG (generowanie liczb losowych) |
| **Narzędzia**| Porównanie (czas stały itp.) |

Domyślna konfiguracja: `cmox_default_config.h`. Opcje dla „szybkiej/małej” implementacji AES: `cmox_fast_config.h`, `cmox_small_config.h`.

### 3.3 Inicjalizacja i warstwa niskiego poziomu

- Na początku aplikacji:
- `cmox_initialize(&init_target)` - z `cmox_init_arg_t` (na przykład `CMOX_INIT_TARGET_AUTO` lub jawnie `CMOX_INIT_TARGET_H7`).
- Wewnętrznie **cmox_ll_init()** jest wywoływana z pliku `interface/cmox_low_level_template.c`.
- W szablonie:
- Włączone jest taktowanie **CRC** (`__HAL_RCC_CRC_CLK_ENABLE()`, itp.) - potrzebne do operacji kryptograficznych.
- Plik musi być dołączony do projektu iw razie potrzeby odkomentowany `#include "stm32h7xx_hal.h"` (lub Twoja seria).
- Po zakończeniu pracy z kryptowalutami:
  - `cmox_finalize(NULL)`.

Bez zaimplementowania `cmox_ll_init`/`cmox_ll_deInit` (i połączenia `cmox_low_level_template.c`) biblioteka nie zostanie poprawnie zainicjalizowana.

---

## 4. Przykład użycia (one-step i streaming)

Z przykładu **AES_CBC_EncryptDecrypt** (NUCLEO-H753ZI):

```c
#include "cmox_crypto.h"

cmox_init_arg_t init_target = { CMOX_INIT_TARGET_AUTO, NULL };

// 1) Inicjalizacja
if (cmox_initialize(&init_target) != CMOX_INIT_SUCCESS) { ... }

// 2) Szyfrowanie jednoetapowe
retval = cmox_cipher_encrypt(CMOX_AES_CBC_ENC_ALGO,
    Plaintext, sizeof(Plaintext), Key, sizeof(Key), IV, sizeof(IV),
    Computed_Ciphertext, &computed_size);

// 3) Lub tryb strumieniowy: konstrukcja → init → setKey → setIV → dołączanie (kawałek po kawałku) → czyszczenie
cipher_ctx = cmox_cbc_construct(&Cbc_Ctx, CMOX_AES_CBC_ENC);
cmox_cipher_init(cipher_ctx);
cmox_cipher_setKey(cipher_ctx, Key, sizeof(Key));
cmox_cipher_setIV(cipher_ctx, IV, sizeof(IV));
cmox_cipher_append(cipher_ctx, chunk, size, out, &out_len);
cmox_cipher_cleanup(cipher_ctx);

// 4) Zakończenie
cmox_finalize(NULL);
```

Podobnie dla AEAD (GCM, CCM, ChaCha20-Poly1305), skrótów, MAC, RSA, ECC, DRBG - poprzez odpowiednie nagłówki i typy (cmox_*_handle_t, cmox_*_construct, cmox_*_encrypt/decrypt itp.).

---

## 5. Przykładowe projekty (Projekty)

Jedna tablica na serię, wewnątrz - zastosowania według kategorii:

- **Cipher**: AES_CBC, AES_GCM_AEAD, ChaCha20-Poly1305_AEAD, SM4_CTR
- **Hash**: SHA2_Digest, SHA3_Digest, SM3_Digest, SHAKE_Digest
- **MAC**: AES_CMAC, HMAC_SHA2, KMAC
- **RSA**: PKCS1v1.5_SignVerify, PKCS1v2.2_SignVerify, PKCS1v2.2_EncryptDecrypt
- **ECC**: ECDSA_SignVerify, ECDH_SharedSecretGeneration, EdDSA_SignVerify, SM2_SignVerify
- **DRBG**: RandomGeneration

Dla **STM32H7** (w tym H743) odpowiedni jest katalog **NUCLEO-H753ZI** (ten sam Cortex-M7, ta sama biblioteka `libSTM32Cryptographic_CM7.a`). Zespoły: **STM32CubeIDE**, **EWARM**, **MDK-ARM** (dla niektórych przykładów - z FSBL dla XIP).

---

## 6. Legacy API (v3)

`legacy_v3/` zawiera opakowania starego API (źródła w C) na wierzchu CMOX dla kompatybilności wstecznej: na przykład `legacy_v3_aes_cbc.c`, `legacy_v3_hmac_sha256.c`, `legacy_v3_ecc.c` itp. W przypadku nowego kodu lepiej jest użyć bezpośredniego API CMOX.

---

## 7. Integracja z projektem (na przykład stm32_secure_boot)

Aby użyć biblioteki kryptograficznej w swoim projekcie (na przykład w celu sprawdzenia podpisu obrazu podczas bezpiecznego rozruchu):

1. **Pakiet Connect**
Określ katalog główny pakietu (na przykład `Crypto_ROOT = /data/projects/STM32CubeExpansion_Crypto_V4.6.0`).

2. **Dodaj do kompilacji**
- Biblioteka: `Middlewares/ST/STM32_Cryptographic/lib/libSTM32Cryptographic_CM7.a` (dla H743).
- Źródło warstwy niskiego poziomu: `Middlewares/ST/STM32_Cryptographic/interface/cmox_low_level_template.c`.
- W `cmox_low_level_template.c` dołącz HAL (na przykład `stm32h7xx_hal.h`) i, jeśli to konieczne, popraw makra RCC dla CRC swojej płyty.

3. **Połącz nagłówki**
   - `Middlewares/ST/STM32_Cryptographic/include`  
   - `Middlewares/ST/STM32_Cryptographic/interface`  
Dołącz `#include "cmox_crypto.h"` do kodu.

4. **Inicjalizacja**
Wywołaj `cmox_initialize()` po HAL (i jeśli to konieczne po ustawieniu zegara/CRC), a po zakończeniu wywołaj `cmox_finalize()`.

5. **Wybór algorytmów**
Aby zweryfikować podpis obrazu, zwykle potrzebujesz: **Hash** (SHA-256 itp.) i **RSA** (PKCS#1 v1.5 lub v2.2) lub **ECC** (ECDSA). Przykłady w Projektach pokazują pełny cykl (klucz, podpis, weryfikacja).

---

## 8. Przydatne linki (z Release_Notes)

- **DB2660** — Databrief STM32 Cryptographic library for STM32Cube.
- **Wiki**: [Kategoria: Biblioteka kryptograficzna](https://wiki.st.com/stm32mcu/wiki/Category:Cryptographic_library) - przegląd, bezpieczne użytkowanie, wydajność, certyfikacja, migracja.
- **NIST ACVP** - łącza do raportów walidacyjnych dla bibliotek podano w uwagach do wydania oprogramowania pośredniego (`Middlewares/ST/STM32_Cryptographic/Release_Notes.html`).

---

## 9. Krótkie podsumowanie

| Element | Opis |
|--------|----------|
| **Wersja pakietu** | V4.6.0 (12 września 2025 r.), oprogramowanie pośredniczące do kryptowalut v 4.0.5 |
| **Biblioteki** | Prekompilowany plik „.a” dla każdego rdzenia (CM0/CM3/CM4/CM7/CM33/CM55) |
| **API** | CMOX (zoptymalizowany dla Cortex-M); opcjonalne starsze_v3 |
| **Inicjalizacja** | `cmox_initialize()` + implementacja `cmox_ll_init()` w `cmox_low_level_template.c` (włącz CRC) |
| **Dla STM32H743** | Użyj przykładów `libSTM32Cryptographic_CM7.a` i NUCLEO-H753ZI |
| **Dokumentacja** | Release_Notes.html w katalogu głównym oraz w `Middlewares/ST/STM32_Cryptographic/`, Wiki ST |

Tego pakietu można użyć w projekcie bezpiecznego rozruchu w celu weryfikacji podpisów (RSA/ECDSA) i skrótów (SHA-256 itp.) podczas ładowania oprogramowania sprzętowego.
