# Сборка и прошивка Secure Bootloader (STM32H743ZI, NUCLEO-144)

Пошаговая инструкция для тех, кто не очень ориентируется в ARM embedded.

---

## 1. Что нужно установить на компьютер

### 1.1 Тулкит ARM GCC (компилятор и линкер)

- **Linux (Debian/Ubuntu):**
  ```bash
  sudo apt update
  sudo apt install gcc-arm-none-eabi
  ```
- **Windows:** скачайте [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/gnu-rm) и добавьте папку `bin` в PATH.
- **macOS:** `brew install arm-none-eabi-gcc`

Проверка:
```bash
arm-none-eabi-gcc --version
```

### 1.2 Python и venv для подписи образа

Уже есть в проекте: `scripts/venv`. Активируйте и проверьте:

```bash
cd /data/projects/stm32_secure_boot
source scripts/venv/bin/activate
python -c "import ecdsa; print('ecdsa OK')"
```

### 1.3 Программатор / прошивка по USB

Плата NUCLEO-144 подключается по USB (ST-Link). Нужна утилита для записи в flash:

- **st-flash** (рекомендуется): [https://github.com/stlink-org/stlink](https://github.com/stlink-org/stlink)  
  Установка (Linux): `sudo apt install stlink-tools` или соберите из исходников.  
  Проверка: `st-info --probe` (должен увидеть чип).

- Либо **STM32CubeProgrammer** (GUI) с сайта ST.

### 1.4 Плата и перемычки (NUCLEO-H743ZI2, MB1364)

**Тип платы:** NUCLEO-H743ZI2 (ревизия MB1364C) — STM32H743, 144-пиновый Nucleo со встроенным ST-LINK/V3.

**Проверьте перемычки:**

| Перемычка | Рекомендация | Назначение |
|-----------|--------------|------------|
| **CN12**  | **2-3** (если питаете только от USB) | Питание целевого MCU: 1-2 = от разъёма EXT, **2-3 = от USB ST-LINK**. Если стоит 1-2 и EXT не подключён — MCU может не питаться. |
| **CN11**  | 1-2 | Питание самого ST-LINK от USB (обычно так и есть). |
| **JP1, JP5** | 1-2 | Загрузка из Flash (нормальный режим). |
| **JP6**    | по умолчанию 1-2 или 2-3 — см. UM2407 | Режим загрузки. |
| **JP7, JP8** | по умолчанию без перемычек | На части плат — опции; на MB1364 VCP к MCU обычно уже соединён на плате. |

**UART (VCP):** виртуальный COM порт ST-LINK на этой плате обычно подключён к **USART3 (PD8 — TX, PD9 — RX)**. В прошивке по умолчанию используется USART2 (PA2/PA3); если логов нет — соберите с `-DUART_LOG_USE_USART3` (см. раздел 6.1).

**Официальная документация:** [User Manual UM2407](https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-boards-mb1364-stmicroelectronics.pdf) (STM32H7 Nucleo-144 boards MB1364).

---

## 2. Карта памяти (как всё лежит во flash)

| Адрес       | Содержимое |
|------------|------------|
| 0x08000000 | Загрузчик (bootloader), 64 KB |
| 0x08010000 | Заголовок образа (image_header_t, 96 байт) |
| 0x08010060 | Код приложения (таблица векторов + программа) |

Бутлоадер при старте читает заголовок по 0x08010000, считает SHA-256 кода приложения, проверяет ECDSA-подпись и при успехе переходит по адресу из поля `entry_point` (0x08010060).

---

## 3. Сборка бутлоадера

Из корня проекта:

```bash
cd /data/projects/stm32_secure_boot
make
```

Или явно:

```bash
make -f bootloader/Makefile all
```

Результат:
- `build/bootloader.elf` — образ для отладки;
- `build/bootloader/bootloader.bin` — бинарник для прошивки в область 0x08000000.

По умолчанию включён **режим stub** (макрос `USE_ECDSA_STUB`): подпись не проверяется, любой образ считается «верным». Так можно быстро проверить цепочку: бутлоадер → переход в приложение. Для настоящей проверки подписи нужна сборка с mbedTLS (см. раздел 7).

---

## 4. Сборка приложения

```bash
make -f app/Makefile all
```

Результат: `build/app/app.bin` — сырой образ приложения (без заголовка), скомпилированный для адреса 0x08010060.

---

## 5. Подпись образа приложения

Скрипт добавляет к `app.bin` заголовок с подписью и выдаёт полный образ для записи начиная с 0x08010000:

```bash
source scripts/venv/bin/activate
python scripts/sign_image.py build/app/app.bin build/app/signed_app.bin
```

Файл `scripts/root_private_key.pem` должен существовать (если ключей ещё нет — см. `scripts/generate_keys.py` и обновите `bootloader/inc/keys.h` открытым ключом).

---

## 6. Прошивка в плату

1. Подключите NUCLEO-144 по USB.
2. Записать бутлоадер по адресу 0x08000000:
   ```bash
   st-flash write build/bootloader/bootloader.bin 0x08000000
   ```
3. Записать подписанное приложение по адресу 0x08010000:
   ```bash
   st-flash write build/app/signed_app.bin 0x08010000
   ```

Либо одной командой (сначала бутлоадер, потом приложение в один объединённый файл — можно подготовить отдельным скриптом).

После сброса платы должен запуститься бутлоадер, затем переход в приложение. На NUCLEO-144 зелёный LED1 (PB0) должен мигать. Если подпись не прошла (при сборке без stub и с mbedTLS), загорится красный LD3 (PB14) и плата «зависнет» в бутлоадере.

**Рекомендуется использовать скрипт** `./scripts/flash.sh` — он собирает один образ и прошивает с 0x08000000 (на STM32H7 страница flash 128 KB, запись только с границы страницы).

---

## 6.1 Логи по UART (консоль)

Бутлоадер и приложение выводят текст в **виртуальный COM порт** ST-Link (USART3, PD8/PD9, **115200 8N1**). Дополнительные провода не нужны — порт уже подключён к разъёму платы.

1. Подключите NUCLEO по USB (тот же кабель, что и для прошивки).
2. В системе появится последовательный порт (например `/dev/ttyACM0` в Linux).
3. Откройте терминал с **115200 8N1** (скорость обязательна):
   ```bash
   minicom -D /dev/ttyACM0 -b 115200
   ```
   или `screen /dev/ttyACM0 115200`. В minicom по умолчанию может быть 9600 — проверьте: **Ctrl+A O** → Serial port setup → **E** (Bps/Par/Bits) → 115200 8N1.
4. Сначала откройте терминал, **затем** нажмите **Reset** на плате — в окне должны появиться строки:
   ```
   [boot] Secure bootloader
   [boot] UART 115200 OK
   Verify: header...
     magic OK
     size OK
     SHA256...
     SHA256 OK
     ECDSA verify...
     ECDSA OK
   [boot] Signature OK
   [boot] Jump to app
   App started
   ```

При ошибке подписи бутлоадер выведет `[boot] Signature FAIL, halt` (и один из `FAIL: bad magic`, `FAIL: image_size=0`, `FAIL: SHA256`, `FAIL: ECDSA`), загорится красный LD3.

По умолчанию бутлоадер собирается с **USART3 (PD8/PD9)** для лога — ST-Link VCP на NUCLEO-H743ZI2.

### Минимальный бутлоадер для проверки лога

Если нужно убедиться, что вывод в UART работает без проверки подписи и перехода в приложение:

1. Соберите минимальный бутлоадер (только инициализация UART и несколько строк в лог):
   ```bash
   make minimal
   ```
2. Прошейте его по адресу 0x08000000:
   ```bash
   st-flash write build/bootloader/bootloader_minimal.bin 0x08000000
   ```
3. Откройте терминал 115200 8N1 и нажмите Reset. Должны появиться строки:
   ```
   === Bootloader log test ===
   If you see this, UART is OK.
   115200 8N1
   ```
4. Чтобы вернуть полный бутлоадер и приложение, снова выполните `./scripts/flash.sh`.

### Standalone-демо: светодиоды + лог (без бутлоадера)

В основном каталоге проекта есть минимальный образ **demo**: по очереди мигают три светодиода (LED1/LED2/LED3), в UART выводится их состояние.

- **Сборка:**
  ```bash
  make demo
  ```
  Результат: `build/demo/demo.bin`.

- **Прошивка** (образ записывается с 0x08000000, бутлоадер не используется):
  ```bash
  st-flash write build/demo/demo.bin 0x08000000
  ```

- **UART:** USART3 (PD8/PD9), 115200 8N1 — тот же виртуальный COM порт ST-Link. В логе по очереди строки вида:
  ```
  Demo: LEDs + log (NUCLEO-H743ZI)
  LED1=ON LED2=OFF LED3=OFF
  LED1=OFF LED2=ON LED3=OFF
  LED1=OFF LED2=OFF LED3=ON
  ```

Подробнее: `demo/README`.

### Официальная прошивка для проверки UART (опционально)

Чтобы убедиться, что плата и виртуальный COM порт работают, можно прошить **официальный пример** из пакета STM32CubeH7:

1. Скачайте [STM32CubeH7](https://github.com/STMicroelectronics/STM32CubeH7/releases) или клонируйте репозиторий.
2. Откройте проект примера UART/COM для NUCLEO-H743ZI, например:
   - `STM32CubeH7/Projects/NUCLEO-H743ZI/Examples/UART/` (если есть UART_Printf или аналог),
   - или любой пример BSP/Examples, где включён вывод в COM (115200).
3. Соберите проект в STM32CubeIDE или с помощью make/CMake из пакета и прошейте полученный `.bin` по адресу **0x08000000**:
   ```bash
   st-flash write <path-to-built>.bin 0x08000000
   ```
4. Откройте minicom/screen на 115200 и нажмите Reset — в терминале должен появиться вывод от примера ST.

Так вы проверите, что питание, перемычки и VCP на плате в порядке. После этого можно снова прошить наш образ через `./scripts/flash.sh`.

---

Сейчас по умолчанию бутлоадер собирается с `-DUSE_ECDSA_STUB` и не проверяет подпись. Чтобы включить проверку:

1. Скачайте mbedTLS в проект, например:
   ```bash
   git clone --depth 1 https://github.com/Mbed-TLS/mbedtls.git third_party/mbedtls
   ```
2. Соберите mbedTLS только с нужными модулями (ECP, ECDSA, Bignum) и получите библиотеку (например, `libmbedtls.a`) или добавьте исходники в сборку бутлоадера.
3. В `bootloader/Makefile` уберите флаг `-DUSE_ECDSA_STUB` и добавьте пути к mbedTLS (`-I third_party/mbedtls/include`) и линковку с `libmbedtls.a`.

После этого бутлоадер будет проверять подпись; при неверной подписи загорится красный LD3 и перехода в приложение не будет.

---

## 8. Краткий чеклист

1. Установить `arm-none-eabi-gcc`, активировать `scripts/venv`, при необходимости установить `st-link`.
2. Собрать бутлоадер: `make`.
3. Собрать приложение: `make -f app/Makefile all`.
4. Подписать образ: `python scripts/sign_image.py build/app/app.bin build/app/signed_app.bin`.
5. Прошить: `st-flash write build/bootloader/bootloader.bin 0x08000000`, затем `st-flash write build/app/signed_app.bin 0x08010000`.
6. Сбросить плату — должно мигать зелёным (приложение запустилось после бутлоадера).

Если что-то не собирается или не прошивается — пришлите текст ошибки и вывод команд.
