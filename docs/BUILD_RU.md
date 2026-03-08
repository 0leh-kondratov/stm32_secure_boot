# Сборка и прошивка Secure Bootloader (STM32H743ZI, NUCLEO-144)

Пошаговая инструкция для тех, кто не очень ориентируется в ARM embedded./.

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
- `build/bootloader.bin` — бинарник для прошивки в область 0x08000000.

По умолчанию включён **режим stub** (макрос `USE_ECDSA_STUB`): подпись не проверяется, любой образ считается «верным». Так можно быстро проверить цепочку: бутлоадер → переход в приложение. Для настоящей проверки подписи нужна сборка с mbedTLS (см. раздел 7).

---

## 4. Сборка приложения

```bash
make -f app/Makefile all
```

Результат: `build_app/app.bin` — сырой образ приложения (без заголовка), скомпилированный для адреса 0x08010060.

---

## 5. Подпись образа приложения

Скрипт добавляет к `app.bin` заголовок с подписью и выдаёт полный образ для записи начиная с 0x08010000:

```bash
source scripts/venv/bin/activate
python scripts/sign_image.py build_app/app.bin build_app/signed_app.bin
```

Файл `scripts/root_private_key.pem` должен существовать (если ключей ещё нет — см. `scripts/generate_keys.py` и обновите `bootloader/inc/keys.h` открытым ключом).

---

## 6. Прошивка в плату

1. Подключите NUCLEO-144 по USB.
2. Записать бутлоадер по адресу 0x08000000:
   ```bash
   st-flash write build/bootloader.bin 0x08000000
   ```
3. Записать подписанное приложение по адресу 0x08010000:
   ```bash
   st-flash write build_app/signed_app.bin 0x08010000
   ```

Либо одной командой (сначала бутлоадер, потом приложение в один объединённый файл — можно подготовить отдельным скриптом).

После сброса платы должен запуститься бутлоадер, затем переход в приложение. На NUCLEO-144 зелёный LED1 (PB0) должен мигать. Если подпись не прошла (при сборке без stub и с mbedTLS), загорится красный LD3 (PB14) и плата «зависнет» в бутлоадере.

---

## 7. (Опционально) Настоящая проверка подписи с mbedTLS

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
4. Подписать образ: `python scripts/sign_image.py build_app/app.bin build_app/signed_app.bin`.
5. Прошить: `st-flash write build/bootloader.bin 0x08000000`, затем `st-flash write build_app/signed_app.bin 0x08010000`.
6. Сбросить плату — должно мигать зелёным (приложение запустилось после бутлоадера).

Если что-то не собирается или не прошивается — пришлите текст ошибки и вывод команд.
