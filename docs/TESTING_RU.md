# Использование примеров для тестирования

Как прогнать имеющиеся примеры и чем вы можете помочь проекту (отчёты о результатах, новые тест-кейсы, скрипты).

**Структура:** приложения (demo, lwip, step1, step2) — в `app/`; тестовые прошивки (stage1 I2C scanner, stage2 FreeRTOS LCD) — в `test/`. Сборка по-прежнему из корня: `make demo`, `make step1`, `make test-stage1` и т.д.

---

## 1. Какие примеры есть

| Пример | Команда сборки | Что прошить | Ожидание после Reset |
|--------|----------------|-------------|----------------------|
| **Официальный шаблон ST** | `make demo-official` | `make flash-demo-official` | 3 LED мигают (без нашего бутлоадера). Нужен клон STM32CubeH7 рядом (`CUBE_ROOT`). |
| **Standalone demo проекта** | `make demo` | `make flash-demo` | 3 LED + лог по UART (115200 8N1, USART3). FreeRTOS, без бутлоадера. |
| **Минимальный бутлоадер** | `make -f bootloader/Makefile minimal-usart3` | `st-flash write build/bootloader_minimal_usart3.bin 0x08000000` | LED1 горит, в UART строка `=== Bootloader log test ===`. |
| **Полный Secure Boot** | `./scripts/flash.sh` | тот же скрипт сам прошивает | LED1 ~1 с, затем лог бутлоадера, затем 3 LED и лог приложения. |

Плата: NUCLEO-H743ZI2 (MB1364). BOOT0 = 0 (загрузка из Flash).

---

## 2. Сначала в Renode (без платы)

Удобно проверить образ в эмуляторе перед прошивкой.

| Образ | Сборка | Запуск Renode | Окно usart3 |
|-------|--------|----------------|-------------|
| **Demo (FreeRTOS)** | `make demo` | `renode demo.resc` | "Demo UART OK", приглашение "> ", команды help, led3 on/off |
| **Бутлоадер + приложение** | `make bootloader signed-app` | `renode simulate.resc` | Лог бутлоадера, затем приложение |

Из корня проекта (где лежат `demo.resc`, `simulate.resc`). GDB: в скриптах поднят сервер на 3334 (demo) или 3333 (simulate), подключаться `target remote :3334` к соответствующему ELF.

### Отладка в Cursor / VS Code (GDB + Renode на localhost)

В проекте есть `.vscode/launch.json` с конфигурациями для подключения к GDB-серверу Renode:

1. **Соберите образ** (один из):
   - `make demo` → для "Debug demo (Renode)"
   - `make bootloader signed-app` → для "Debug bootloader+app (Renode simulate)"

2. **Запустите Renode** в терминале из корня проекта:
   - для demo: `renode demo.resc`
   - для бутлоадера: `renode simulate.resc`

3. **В Cursor:** Run → Start Debugging (F5), выберите конфигурацию (demo или bootloader+app). Отладчик подключится к `localhost:3334` или `localhost:3333` и подгрузит символы из соответствующего `.elf`.

Нужно расширение **C/C++** (ms-vscode.cpptools). Если `arm-none-eabi-gdb` не в PATH, укажите полный путь в `launch.json` в поле `miDebuggerPath`.

### Отладка на железе (ST-Link, данные debug в Cursor)

Чтобы получать данные отладки (breakpoints, переменные, call stack) с реальной платы:

1. **Прошейте образ** на плату (один раз):  
   `make flash-demo`.

2. **Запустите GDB-сервер ST-Link** в отдельном терминале:
   ```bash
   st-util
   ```
   Сервер слушает порт **61234**. Плата должна быть подключена по USB (ST-Link).

3. **В Cursor:** Run → Start Debugging (F5), выберите **Debug demo (ST-Link, железо)**.

4. Отладчик подключится к плате, подгрузит символы из `.elf` и остановится на входе в `main` (если `stopAtEntry: true`). Дальше: ставить breakpoint’ы, смотреть переменные, пошагово выполнять код. Вывод UART по-прежнему смотрите в minicom.

**Запуск Renode:** из корня проекта (`cd /data/projects/stm32_secure_boot`), иначе скрипты и пути к `build/` не найдутся.

В скриптах стоит `logLevel 3` — в консоли выводятся только ошибки; предупреждения RCC/peripheral (нереализованные регистры модели STM32H743) скрыты. Окно **usart3** должно показывать лог, даже если в консоли были предупреждения.

---

## 3. Как прогнать тесты по шагам

### 2.1 Проверка окружения (без платы)

Запустите скрипт, который собирает все цели (без прошивки):

```bash
cd /data/projects/stm32_secure_boot
./scripts/build_all.sh
```

Или вручную:

```bash
cd /data/projects/stm32_secure_boot
make clean && make bootloader
make -f app/Makefile all
make signed-app
make demo
make -f bootloader/Makefile minimal-usart3
```

Все цели должны собираться без ошибок. Если что-то падает — пришлите вывод `make` и версию `arm-none-eabi-gcc --version`.

### 2.2 С платой: порядок проверки

1. **Подключите NUCLEO по USB**, проверьте:  
   `st-info --probe`  
   Должен быть один программер (ST-Link).

2. **Официальный шаблон (базовая проверка платы):**  
   `make demo-official && make flash-demo-official`  
   Reset → должны мигать 3 LED. Если нет — проверьте BOOT0 и питание.

3. **Standalone demo (наш код, без бутлоадера):**  
   `make demo`  
   `st-flash write build/demo/demo.bin 0x08000000`  
   Reset → 3 LED + лог в minicom/putty 115200 8N1 (USART3 / VCP).

4. **Минимальный бутлоадер (только лог + LED1):**  
   `make -f bootloader/Makefile minimal-usart3`  
   `st-flash write build/bootloader_minimal_usart3.bin 0x08000000`  
   Reset → LED1 горит, в UART: `=== Bootloader log test ===`.

5. **Полный сценарий Secure Boot:**  
   `./scripts/flash.sh`  
   Reset → сначала LED1 и лог бутлоадера, затем мигание 3 LED и лог приложения.

После каждого шага можно записать: что увидели (LED / UART / ошибка) и при какой плате/прошивке.

---

## 4. Чем вы можете помочь

### 3.1 Отчёт о результатах тестирования

Запустите сценарии из п. 2 и пришлите краткий отчёт в формате:

- **Плата:** NUCLEO-H743ZI2 (или другая).
- **Окружение:** ОС, версия `arm-none-eabi-gcc`, `st-flash`, Python.
- **Результаты по пунктам 2.2:**  
  например: «2 — OK, 3 — OK, 4 — LED есть, UART 0 байт, 5 — после Reset LED и лог пропадают».

Это поможет понять, на каких конфигурациях что работает, и воспроизвести проблемы.

### 3.2 Дополнительные тест-кейсы

Полезные сценарии, которые можно добавить или описать:

- Прошивка **только приложения** по 0x08010000 (без бутлоадера) — ожидаемо не должна запускаться с 0x08000000; описание ожидания помогает новичкам.
- Прошивка **повреждённого** или **неподписанного** образа по 0x08010000 при сохранённом бутлоадере — бутлоадер должен отказать в запуске (LED3 / halt).
- Проверка на **другой плате** той же серии (например NUCLEO-H743ZI без «2») или другой ОС (Windows/macOS).

Если вы опишете такой сценарий и результат (шаги + ожидание + факт), его можно внести в этот документ или в CI/скрипты.

### 3.3 Скрипты и автоматизация

- **Скрипт «прогнать все сборки»** (без прошивки): например `scripts/build_all.sh`, который вызывает `make clean`, `make bootloader`, `make app`, `make signed-app`, `make demo`, `make -f bootloader/Makefile minimal-usart3` и выводит OK/FAIL.
- **Чеклист в виде файла** (например `test/checklist.txt` или таблица в Markdown), который вы заполняете после ручных тестов и прикладываете к issue/PR.

Такие скрипты и шаблоны чеклиста можно оформить как PR в репозиторий.

### 3.4 Документация и воспроизведение багов

- Если что-то не сходится с [BUILD_RU.md](BUILD_RU.md) или с этим файлом — предложите правку (конкретная команда, шаг, ожидание).
- При «странном» поведении (тишина в UART, LED не горят после Reset и т.п.) полезно указать: что прошивали последним, делали ли полное питание после прошивки или только Reset, BOOT0.

---

## 5. Краткая шпаргалка команд

```bash
# Сборка всего (без прошивки)
make clean && make bootloader && make signed-app && make demo
make -f bootloader/Makefile minimal-usart3

# Прошивка полного Secure Boot
./scripts/flash.sh

# Прошивка только standalone demo
make demo && st-flash write build/demo/demo.bin 0x08000000

# Прошивка только минимального бутлоадера
make -f bootloader/Makefile minimal-usart3
st-flash write build/bootloader_minimal_usart3.bin 0x08000000
```

UART: 115200 8N1, USART3 (на NUCLEO часто через ST-Link VCP, например `/dev/ttyACM0`).
