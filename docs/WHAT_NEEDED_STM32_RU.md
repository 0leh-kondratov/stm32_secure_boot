# Что нужно для создания и прошивки образа под STM32

Кратко: что установить и какие шаги выполнить, чтобы **собрать** и **прошить** образ (в том числе с проверкой подписи) — в **нашем проекте** и при использовании **официальных решений ST**.

---

## 1. В этом проекте (stm32_secure_boot)

Минимальный набор для сборки подписанного образа и прошивки в NUCLEO-H743ZI2.

### 1.1 Что нужно установить

| Компонент | Назначение | Где взять / как установить |
|-----------|------------|----------------------------|
| **ARM GCC** | Компиляция бутлоадера и приложения | Linux: `sudo apt install gcc-arm-none-eabi`. Windows: [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm). macOS: `brew install arm-none-eabi-gcc`. |
| **Python 3** | Подпись образа (ECDSA) | Системный Python или venv проекта `scripts/venv`. В проекте уже есть `ecdsa` в venv. |
| **st-flash** | Запись образа во Flash по USB | Linux: `sudo apt install stlink-tools`. Или сборка из [stlink-org/stlink](https://github.com/stlink-org/stlink). |
| **Плата** | Целевое устройство | NUCLEO-H743ZI2 (MB1364) с подключением по USB (встроенный ST-Link). |

Дополнительно: **minicom** или **putty** для просмотра UART (115200 8N1), если нужен лог.

### 1.2 Создание и прошивка образа

**Сборка образа (бутлоадер + подписанное приложение):**

```bash
cd /path/to/stm32_secure_boot
make bootloader
make signed-app
```

Получаете: `build/bootloader/bootloader.bin`, `build/app/app.bin`, `build/app/signed_app.bin`.

**Прошивка одним скриптом (рекомендуется):**

```bash
./scripts/flash.sh
```

Скрипт сам собирает бутлоадер, приложение, подписывает и записывает объединённый образ с 0x08000000.

**Либо вручную:** объединить бутлоадер (64 KB) + signed_app.bin и прошить (см. [BUILD_RU.md](BUILD_RU.md)).

Подробная пошаговая инструкция: [BUILD_RU.md](BUILD_RU.md).

---

## 2. Официальные решения ST

Для создания и прошивки образа в рамках **X-CUBE-SBSFU** или с **Trusted Package Creator** нужны другие инструменты и порядок действий.

### 2.1 X-CUBE-SBSFU (Secure Boot + Secure Firmware Update)

**Что это:** пакет расширения STM32Cube с готовым бутлоадером, проверкой подписи (RSA/ECDSA) и обновлением по UART (Ymodem). Поддерживается STM32H7 (в т.ч. H743).

**Что нужно:**

| Компонент | Назначение |
|-----------|------------|
| **X-CUBE-SBSFU** | Сам пакет с исходниками и примерами. |
| **STM32CubeMX** (опционально) | Генерация кода под вашу плату и настройка SBSFU. |
| **Среда сборки** | STM32CubeIDE (рекомендуется ST) или Makefile/IAR/Keil — по примерам из пакета. |
| **Документация** | User Manual **DM00414687** (Getting started), Application Note **DM00414677** (Integration guide). |

**Шаги (общая схема):**

1. Скачать [X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) с st.com (нужна учётная запись ST).
2. Распаковать пакет, открыть пример под вашу серию (например STM32H7) в STM32CubeIDE или собрать по инструкции из DM00414687.
3. Сгенерировать ключи и подписать образ приложения по процедуре из пакета (скрипты/утилиты идут в комплекте).
4. Прошить: сначала бутлоадер SBSFU, затем приложение — способ зависит от примера (STM32CubeProgrammer, загрузчик по UART и т.д.).

Конкретные команды и пункты меню см. в **DM00414687** и **DM00414677**.

### 2.2 STM32 Trusted Package Creator (подпись и упаковка образов)

**Что это:** утилита для подписи и упаковки образов (в т.ч. для Secure Firmware Install). Идёт в составе **STM32CubeProgrammer**.

**Что нужно:**

| Компонент | Назначение |
|-----------|------------|
| **STM32CubeProgrammer** | Включает GUI программера и **Trusted Package Creator**. |
| **Документация** | User Manual **UM2238** (STM32 Trusted Package Creator). |

**Где взять:** [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) (st.com, раздел Development Tools).

**Шаги (общая схема):**

1. Установить STM32CubeProgrammer (Linux/Windows/macOS).
2. Запустить **Trusted Package Creator** (из того же пакета или из меню программера — см. UM2238).
3. Загрузить ваш образ приложения (.bin/.hex), задать ключи и параметры подписи/шифрования по UM2238.
4. Получить подписанный/запакованный образ и прошить его через STM32CubeProgrammer или через бутлоадер (например SBSFU), в зависимости от выбранного сценария.

Формат образа и опции (RSA, ECDSA, AES-GCM и т.д.) описаны в **UM2238**.

### 2.3 SBSFU by MCUboot (L5, U5, H5, H7RS — не для классического H743)

Для **STM32H743** официальный путь с готовыми примерами — **X-CUBE-SBSFU**. Решения на базе MCUboot (OEMiRoT/OEMuRoT) в Cube поставляются для других серий (L5, U5, WBA5, H5, H7RS), не для обычного H743.

### 2.4 Application Notes (общее понимание)

- **AN5447** — обзор Secure Boot и Secure Firmware Update (TrustZone и др.).
- **AN4992** — введение в Secure Firmware Install (SFI).

Полезны для архитектуры и терминологии; прямые пошаговые инструкции «как прошить» — в User Manual пакетов (DM00414687, UM2238).

---

## 3. Сводная таблица

| Действие | Этот проект | X-CUBE-SBSFU | Trusted Package Creator |
|----------|-------------|--------------|--------------------------|
| **Сборка образа** | `make bootloader` + `make app` | STM32CubeIDE / Make по примерам пакета | Не собирает, только подписывает готовый .bin |
| **Подпись** | `scripts/sign_image.py` (Python + ecdsa) | Инструменты/скрипты из пакета SBSFU | Trusted Package Creator (GUI) |
| **Прошивка** | `st-flash` или `./scripts/flash.sh` | STM32CubeProgrammer или встроенный загрузчик по UART | STM32CubeProgrammer (или после подписи — через SBSFU) |
| **Документация** | [BUILD_RU.md](BUILD_RU.md) | DM00414687, DM00414677 | UM2238 |

---

## 4. Итог

- **Чтобы создать и прошить образ в нашем проекте:** установите ARM GCC, Python (и зависимости для подписи), st-flash; плата NUCLEO-H743ZI2 по USB. Дальше: `./scripts/flash.sh` или шаги из [BUILD_RU.md](BUILD_RU.md).
- **Чтобы использовать официальные решения ST:** скачайте **X-CUBE-SBSFU** и/или **STM32CubeProgrammer** (с Trusted Package Creator), откройте **DM00414687** и **UM2238** и действуйте по соответствующим разделам для вашей платы и сценария.

**Пошаговый быстрый старт по X-CUBE-SBSFU (скачать → распаковать → найти проект H7 → собрать и прошить):** см. [SBSFU_QUICKSTART_RU.md](SBSFU_QUICKSTART_RU.md).

Если нужны прямые ссылки на PDF (DM00414687, DM00414677, UM2238, AN5447, AN4992), их можно найти на [st.com](https://www.st.com) в разделе документации к [X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) и [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).
