# Официальные и похожие решения: Secure Boot для STM32

Краткий обзор референсов и официальных примеров ST и сообщества — для сравнения с текущим минимальным бутлоадером (STM32H743, ECDSA P-256 + SHA-256).

**Терминология и концепции Verified Boot применительно к STM32:** см. [VERIFIED_BOOT_STM32_RU.md](VERIFIED_BOOT_STM32_RU.md).

**Что нужно для создания и прошивки образа (наш проект и официальные ST):** см. [WHAT_NEEDED_STM32_RU.md](WHAT_NEEDED_STM32_RU.md).

---

## 1. Официальные решения ST

### 1.1 X-CUBE-SBSFU (Secure Boot and Secure Firmware Update)

- **Что это:** пакет расширения STM32Cube — полноценный secure boot + безопасное обновление прошивки.
- **Где:** [ST X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) (скачать с st.com).
- **Поддерживаемые серии:** в том числе **STM32H7** (включая STM32H743), G0, G4, F4, F7, L0, L1, L4, L4+, WB, WL.
- **Возможности:**
  - проверка подлинности и целостности приложения перед запуском;
  - обновление по UART (Ymodem) и др.;
  - асимметричная (RSA, ECDSA) или симметричная криптография;
  - опционально шифрование образа, интеграция с STSAFE-A110.
- **Документация:**
  - User Manual: DM00414687 (Getting started with X-CUBE-SBSFU);
  - Application Note: DM00414677 (Integration guide);
  - Technical Note: TN1387 (Security evaluation).

**Плюсы:** эталонное решение ST, много плат. **Минусы:** тяжёлое, под свою структуру проекта и память.

---

### 1.2 SBSFU by MCUboot (новое направление ST)

- **Что это:** реализация secure boot на базе открытого **MCUboot**.
- **Вики ST:** [SBSFU by MCUboot](https://wiki.st.com/stm32mcu/wiki/Security:SBSFU_by_MCUboot).
- **Где примеры:** в пакетах STM32Cube для **STM32L5, STM32U5, STM32WBA5** (TF-M), а также **STM32U0, STM32H5, STM32H7RS** (OEMiRoT / OEMuRoT).
- **Для STM32H7 «классических» (H743 и т.п.):** основной официальный пример по secure boot — по-прежнему **X-CUBE-SBSFU** (п. 1.1). OEMiRoT/OEMuRoT ориентированы на H7RS (H7R/H7S с TrustZone).

---

### 1.3 STM32 Trusted Package Creator

- **Что это:** утилита из комплекта **STM32CubeProgrammer** для подписи и упаковки образов.
- **Где:** идёт вместе с [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).
- **Функции:** подпись образа, Secure Firmware Install (SFI), шифрование (AES-GCM), подготовка пакетов для производства.
- **Документация:** User Manual UM2238 (STM32 Trusted Package Creator).

Можно использовать как альтернативу своему скрипту подписи (например, `scripts/sign_image.py`) для совместимости с инструментами ST.

---

### 1.4 Application Notes ST по безопасности

- **AN5447** — обзор Secure Boot и Secure Firmware Update на Arm TrustZone для STM32.
- **AN4992** — введение в Secure Firmware Install (SFI) для STM32.

Имеют смысл для общего понимания архитектуры и терминологии ST.

---

## 2. Открытые референсы

### 2.1 3mdeb/verified-boot (концепции и терминология)

- **Репозиторий:** [3mdeb/verified-boot](https://github.com/3mdeb/verified-boot) (форк adrelanos/verified-boot).
- **Что это:** база знаний по **Verified Boot** — документирование и разбор механизмов проверки загрузки в индустрии. Ориентир на надёжные источники (NIST, TCG) и единую терминологию.
- **Содержимое:**
  - **Trust, Root of Trust (RoT), Chain of Trust (CoT), TCB** — определения.
  - **RTV (Root of Trust for Verification)** — «неизменяемый код (например boot ROM), который криптографически проверяет первый изменяемый код до его выполнения» (цит. по ARM BBSR). Текущий бутлоадер в проекте по сути минимальный RTV.
  - **Verified Boot vs Measured Boot** — verified проверяет подпись/образ *до* запуска и может блокировать непроверенный код; measured только *фиксирует*, что было запущено (для последующего анализа и аттестации).
  - **Целостность (integrity) и подлинность (authenticity)** — хеш даёт целостность; подпись (ECDSA/RSA) даёт подлинность и неотказуемость (non-repudiation).
- **Охват в репо:** в основном PC/UEFI и Linux (UEFI Secure Boot, Intel Boot Guard, dm-verity, LUKS, TPM). Цель — образование и возможный открытый стандарт Verified Boot для Linux (Kicksecure).
- **Для STM32:** прямых примеров под микроконтроллеры нет, но определения RoT, RTV, verified vs measured и «integrity + authenticity» применимы к любому secure boot, в том числе к минимальному бутлоадеру с ECDSA.

Документы в репо: [Verified Boot (main)](https://github.com/3mdeb/verified-boot/blob/master/verified_boot_main.md), [Threat Model](https://github.com/3mdeb/verified-boot/blob/master/threat-model.md), [Measured Boot](https://github.com/3mdeb/verified-boot/blob/master/measured_boot.md). Отдельно — [verified_boot_app_spec.md](https://github.com/3mdeb/verified-boot/blob/master/verified_boot_app_spec.md) (спека UEFI-приложения «Sovereign Boot Provisioning Wizard» для настройки UEFI Secure Boot пользователем).

### 2.2 MCUboot (upstream)

- **Сайт:** [docs.mcuboot.com](https://docs.mcuboot.com/).
- **Репозиторий:** [mcu-tools/mcuboot](https://github.com/mcu-tools/mcuboot).
- **Что даёт:** единый формат образа (заголовок, TLV, подпись), поддержка ECDSA P-256, RSA, Ed25519, SHA-256/384/512, шифрование образов.
- **Порт ST для STM32:** [STMicroelectronics/stm32-mw-mcuboot](https://github.com/STMicroelectronics/stm32-mw-mcuboot) — можно смотреть структуру проекта и интеграцию с STM32.

Текущий проект по идее ближе к «минимальному своему бутлоадеру + свой формат заголовка», но формат и логику MCUboot можно использовать как референс (заголовок, TLV, порядок проверки).

---

### 2.3 STM32H7RS / OEMiRoT (для новых H7 с TrustZone)

- **Вики:** [OEMiRoT for STM32H7R](https://wiki.st.com/stm32mcu/wiki/Security:OEMiRoT_for_STM32H7R), [Security features on STM32H7RS](https://wiki.st.com/stm32mcu/wiki/Security:Security_features_on_STM32H7RS_MCUs).
- **Актуально для:** STM32H7R/H7S (H7RS), не для обычного H743. На H743 официальный путь — X-CUBE-SBSFU или свой минимальный бутлоадер, как в этом репозитории.

---

## 3. Сравнение с текущим проектом

| Аспект              | Текущий проект (stm32_secure_boot) | X-CUBE-SBSFU      | MCUboot / ST port   |
|---------------------|-----------------------------------|-------------------|---------------------|
| Платформа           | STM32H743ZI (NUCLEO-144)          | Много серий, в т.ч. H7 | Разные MCU, порты под STM32 |
| Подпись             | ECDSA P-256 + SHA-256 (пока stub)  | RSA/ECDSA/симметричная | ECDSA, RSA, Ed25519 |
| Обновление по воздуху | Нет                              | Да (Ymodem и др.) | Да (разные транспорты) |
| Сложность           | Минимальная                       | Высокая           | Средняя             |
| Формат образа       | Свой (image_header_t + app)        | Свой              | Стандартный MCUboot |

---

## 4. Что взять за основу для развития

- **Оставаться минимальным:** дорабатывать текущий бутлоадер (реальная ECDSA, защита ключа, при необходимости — формат, совместимый с TLV как в MCUboot).
- **Нужен «как у ST»:** скачать **X-CUBE-SBSFU**, собрать пример под STM32H7 и перенести идеи (разметка flash, порядок проверки, при желании — формат заголовка).
- **Нужен стандартный формат и обновление:** посмотреть **STMicroelectronics/stm32-mw-mcuboot** и либо портировать MCUboot под H743, либо привести свой образ к формату MCUboot и использовать их bootutil только для проверки.

Если нужны прямые ссылки на PDF (AN5447, UM2238 и т.д.), их можно найти по названию на [st.com](https://www.st.com) в разделе Documentation к соответствующему продукту.
