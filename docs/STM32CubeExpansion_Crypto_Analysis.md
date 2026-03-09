# Анализ пакета STM32CubeExpansion_Crypto V4.6.0

Пакет расположен по пути: `/data/projects/STM32CubeExpansion_Crypto_V4.6.0` (в запросе указан путь с опечаткой `...V4.6.0./` — лишняя точка в конце).

---

## 1. Назначение

**X-CUBE-CRYPTOLIB** — криптографическая прошивка (firmware) для всех серий STM32 от STMicroelectronics:

- Алгоритмы проверены в рамках **NIST ACVP** (сертификация).
- Поддержка **Cortex-M0/M0+, M3, M4, M7, M33, M55**.
- Сборка под **GCC, ARM Compiler, IAR EWARM**.

---

## 2. Структура каталогов

```
STM32CubeExpansion_Crypto_V4.6.0/
├── Release_Notes.html      # История изменений пакета
├── Package_license.html
├── sbom_cdx.json           # SBOM (CycloneDX v1.5)
├── Drivers/                 # HAL, CMSIS, BSP по сериям (G0, G4, H5, H7, H7RS, L0–L5, N6, U0/U3/U5, WB, WBA, WL и др.)
├── Middlewares/
│   └── ST/
│       ├── STM32_Cryptographic/   # Основная крипто-библиотека (v4.0.5)
│       ├── STM32_ExtMem_Loader/   # Загрузчик во внешнюю память
│       └── STM32_ExtMem_Manager/  # Менеджер внешней памяти (NOR, PSRAM, SD и т.д.)
└── Projects/               # Примеры по платам (NUCLEO-*, P-NUCLEO-WB55)
```

---

## 3. Криптографическая библиотека (STM32_Cryptographic)

### 3.1 Реализация

- **Библиотеки** — прекомпилированные `.a` в `Middlewares/ST/STM32_Cryptographic/lib/`:
  - `libSTM32Cryptographic_CM0_CM0PLUS.a`
  - `libSTM32Cryptographic_CM3.a`
  - `libSTM32Cryptographic_CM4.a`
  - `libSTM32Cryptographic_CM7.a`   ← **для STM32H743 (Cortex-M7)**
  - `libSTM32Cryptographic_CM33.a`
  - `libSTM32Cryptographic_CM55.a`  (в т.ч. для STM32N6)
- Исходников алгоритмов в пакете нет — только заголовки и **legacy_v3** (обёртки старого API).

### 3.2 API (CMOX — Cortex-M Optimized)

Единая точка входа по заголовкам: `#include "cmox_crypto.h"` (подтягивает все модули).

| Модуль   | Заголовки / алгоритмы |
|----------|------------------------|
| **Cipher** | AES (CBC, CCM, CFB, CTR, ECB, GCM, OFB, XTS, Keywrap), SM4, ChaCha20-Poly1305 |
| **Hash** | SHA-1, SHA-224/256/384/512, SHA-3, SM3, SHAKE |
| **MAC**  | CMAC, HMAC, KMAC |
| **RSA**  | PKCS#1 v1.5, PKCS#1 v2.2 (Encrypt/Decrypt, Sign/Verify) |
| **ECC**  | ECDSA, EdDSA, SM2, ECDH |
| **DRBG** | CTR_DRBG (генерация случайных чисел) |
| **Utils**| Сравнение (constant-time и др.) |

Конфигурация по умолчанию: `cmox_default_config.h`. Варианты «быстрая/малая» реализация AES: `cmox_fast_config.h`, `cmox_small_config.h`.

### 3.3 Инициализация и низкоуровневый слой

- В начале работы приложения:
  - `cmox_initialize(&init_target)` — с `cmox_init_arg_t` (например `CMOX_INIT_TARGET_AUTO` или явно `CMOX_INIT_TARGET_H7`).
  - Внутри вызывается **cmox_ll_init()** из файла `interface/cmox_low_level_template.c`.
- В шаблоне:
  - Включается тактирование **CRC** (`__HAL_RCC_CRC_CLK_ENABLE()` и т.п.) — нужно для крипто-операций.
  - Файл нужно включить в проект и при необходимости раскомментировать `#include "stm32h7xx_hal.h"` (или своей серии).
- По окончании работы с крипто:
  - `cmox_finalize(NULL)`.

Без реализации `cmox_ll_init`/`cmox_ll_deInit` (и линковки `cmox_low_level_template.c`) библиотека не будет корректно инициализирована.

---

## 4. Пример использования (одношаговый и потоковый)

Из примера **AES_CBC_EncryptDecrypt** (NUCLEO-H753ZI):

```c
#include "cmox_crypto.h"

cmox_init_arg_t init_target = { CMOX_INIT_TARGET_AUTO, NULL };

// 1) Инициализация
if (cmox_initialize(&init_target) != CMOX_INIT_SUCCESS) { ... }

// 2) Одношаговое шифрование
retval = cmox_cipher_encrypt(CMOX_AES_CBC_ENC_ALGO,
    Plaintext, sizeof(Plaintext), Key, sizeof(Key), IV, sizeof(IV),
    Computed_Ciphertext, &computed_size);

// 3) Или потоковый режим: construct → init → setKey → setIV → append (по кускам) → cleanup
cipher_ctx = cmox_cbc_construct(&Cbc_Ctx, CMOX_AES_CBC_ENC);
cmox_cipher_init(cipher_ctx);
cmox_cipher_setKey(cipher_ctx, Key, sizeof(Key));
cmox_cipher_setIV(cipher_ctx, IV, sizeof(IV));
cmox_cipher_append(cipher_ctx, chunk, size, out, &out_len);
cmox_cipher_cleanup(cipher_ctx);

// 4) Завершение
cmox_finalize(NULL);
```

Аналогично для AEAD (GCM, CCM, ChaCha20-Poly1305), хешей, MAC, RSA, ECC, DRBG — через соответствующие заголовки и типы (cmox_*_handle_t, cmox_*_construct, cmox_*_encrypt/decrypt и т.д.).

---

## 5. Проекты-примеры (Projects)

По одной плате на серию, внутри — приложения по категориям:

- **Cipher**: AES_CBC, AES_GCM_AEAD, ChaCha20-Poly1305_AEAD, SM4_CTR
- **Hash**: SHA2_Digest, SHA3_Digest, SM3_Digest, SHAKE_Digest
- **MAC**: AES_CMAC, HMAC_SHA2, KMAC
- **RSA**: PKCS1v1.5_SignVerify, PKCS1v2.2_SignVerify, PKCS1v2.2_EncryptDecrypt
- **ECC**: ECDSA_SignVerify, ECDH_SharedSecretGeneration, EdDSA_SignVerify, SM2_SignVerify
- **DRBG**: RandomGeneration

Для **STM32H7** (в т.ч. H743) релевантен каталог **NUCLEO-H753ZI** (тот же Cortex-M7, та же библиотека `libSTM32Cryptographic_CM7.a`). Сборки: **STM32CubeIDE**, **EWARM**, **MDK-ARM** (для части примеров — с FSBL для XIP).

---

## 6. Legacy API (v3)

В `legacy_v3/` лежат обёртки старого API (исходники в C) поверх CMOX для обратной совместимости: например `legacy_v3_aes_cbc.c`, `legacy_v3_hmac_sha256.c`, `legacy_v3_ecc.c` и т.д. Для нового кода предпочтительно использовать прямой CMOX API.

---

## 7. Интеграция в проект (например, stm32_secure_boot)

Чтобы использовать криптобиблиотеку в своём проекте (например, для проверки подписи образа при secure boot):

1. **Подключить пакет**  
   Указать корень пакета (например `Crypto_ROOT = /data/projects/STM32CubeExpansion_Crypto_V4.6.0`).

2. **Добавить в сборку**  
   - Библиотека: `Middlewares/ST/STM32_Cryptographic/lib/libSTM32Cryptographic_CM7.a` (для H743).  
   - Исходник низкоуровневого слоя: `Middlewares/ST/STM32_Cryptographic/interface/cmox_low_level_template.c`.  
   - В `cmox_low_level_template.c` включить свой HAL (например `stm32h7xx_hal.h`) и при необходимости поправить макросы RCC для CRC под свою плату.

3. **Подключить заголовки**  
   - `Middlewares/ST/STM32_Cryptographic/include`  
   - `Middlewares/ST/STM32_Cryptographic/interface`  
   Включать в коде `#include "cmox_crypto.h"`.

4. **Инициализация**  
   Вызвать `cmox_initialize()` после HAL (и при необходимости после настройки тактирования/CRC), по окончании — `cmox_finalize()`.

5. **Выбор алгоритмов**  
   Для проверки подписи образа обычно нужны: **Hash** (SHA-256 и т.п.) и **RSA** (PKCS#1 v1.5 или v2.2) или **ECC** (ECDSA). Примеры в Projects показывают полный цикл (ключ, подпись, верификация).

---

## 8. Полезные ссылки (из Release_Notes)

- **DB2660** — Databrief STM32 Cryptographic library for STM32Cube.
- **Wiki**: [Category: Cryptographic library](https://wiki.st.com/stm32mcu/wiki/Category:Cryptographic_library) — обзор, безопасное использование, производительность, сертификация, миграция.
- **NIST ACVP** — ссылки на отчёты валидации по библиотекам указаны в Release Notes middleware’а (`Middlewares/ST/STM32_Cryptographic/Release_Notes.html`).

---

## 9. Краткое резюме

| Элемент | Описание |
|--------|----------|
| **Версия пакета** | V4.6.0 (12-Sep-2025), крипто-middleware v4.0.5 |
| **Библиотеки** | Прекомпилированные `.a` под каждое ядро (CM0/CM3/CM4/CM7/CM33/CM55) |
| **API** | CMOX (Cortex-M Optimized); опционально legacy_v3 |
| **Инициализация** | `cmox_initialize()` + реализация `cmox_ll_init()` в `cmox_low_level_template.c` (включить CRC) |
| **Для STM32H743** | Использовать `libSTM32Cryptographic_CM7.a` и примеры NUCLEO-H753ZI |
| **Документация** | Release_Notes.html в корне и в `Middlewares/ST/STM32_Cryptographic/`, Wiki ST |

Этот пакет можно использовать в проекте secure boot для проверки подписей (RSA/ECDSA) и хешей (SHA-256 и др.) при загрузке прошивки.
