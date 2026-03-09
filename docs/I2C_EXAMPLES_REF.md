# Готовые примеры I2C (STM32CubeH7 и NUCLEO-H743ZI)

## Где искать

В **STM32CubeH7** для платы **NUCLEO-H743ZI** отдельной папки `Examples/I2C` нет. Удобнее всего опираться на примеры для **NUCLEO-H723ZG** — тот же ряд H7, те же пины I2C1.

---

## 1. NUCLEO-H723ZG — I2C1, PB8/PB9 (совместимо с NUCLEO-H743ZI)

Путь:  
`STM32CubeH7/Projects/NUCLEO-H723ZG/Examples/I2C/`

| Пример | Описание |
|--------|----------|
| **I2C_TwoBoards_ComPolling** | Master/Slave, опрос (polling). Инициализация через `HAL_I2C_Init`, расчёт TIMING через `I2C_GetTiming()`. |
| I2C_TwoBoards_ComIT | Обмен по прерываниям. |
| I2C_TwoBoards_ComDMA | Обмен по DMA. |
| I2C_WakeUpFromStop | Пробуждение из Stop по I2C. |

Пины (из readme):

- **SCL:** PB8 (CN7, D15)  
- **SDA:** PB9 (CN7, D14)  
- **Интерфейс:** I2C1  

То есть те же PB8/PB9, что и в нашем Step 1 для NUCLEO-H743ZI.

Ключевые файлы:

- `Src/main.c` — вызов `HAL_I2C_Init`, `HAL_I2C_Master_Transmit` / `HAL_I2C_Master_Receive` (или Slave-аналоги).
- `Src/stm32h7xx_hal_msp.c` — `HAL_I2C_MspInit`: GPIO (PB8/PB9, AF4), включение тактирования I2C1 и GPIOB.
- `Inc/main.h` — макросы `I2Cx` (I2C1), пины, порты, AF.
- `Src/i2c_timing_utility.c`, `Inc/i2c_timing_utility.h` — расчёт регистра **TIMING** по частоте шины и PCLK (важно для стабильной работы на H7).

Инициализация I2C в примере (фрагмент):

```c
I2cHandle.Instance            = I2C1;
I2cHandle.Init.Timing          = I2C_GetTiming(HAL_RCC_GetPCLK2Freq(), BUS_I2Cx_FREQUENCY);
I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_10BIT;
// ... остальные поля
HAL_I2C_Init(&I2cHandle);
HAL_I2CEx_ConfigAnalogFilter(&I2cHandle, I2C_ANALOGFILTER_ENABLE);
```

На **STM32H743** I2C1 висит на **APB1**, поэтому при переносе на H743 нужно использовать `HAL_RCC_GetPCLK1Freq()` вместо `HAL_RCC_GetPCLK2Freq()` при вызове `I2C_GetTiming()`.

---

## 2. STM32H7B3I-EVAL — I2C и EEPROM

Путь:  
`STM32CubeH7/Projects/STM32H7B3I-EVAL/Examples/I2C/I2C_EEPROM_fast_mode_plus/`

- Обмен с EEPROM M24LR64 по I2C (Fast Mode Plus).
- Используется фиксированный TIMING и DMA.
- Плата и пины другие, но полезно как эталон работы с HAL I2C и таймингом.

---

## 3. Наш Step 1 (stm32_secure_boot)

- **I2C:** I2C1, PB8 (SCL), PB9 (SDA) — как в примере H723ZG.
- В `app/step1/main.c` инициализация через `MX_I2C1_Init()` с **захардкоженным** `Timing = 0x10909CEC` (100 kHz при 200 MHz APB1).
- Если после смены частоты или платы I2C ведёт себя нестабильно, имеет смысл перенести в проект утилиту расчёта тайминга из NUCLEO-H723ZG и вызывать:
  - `I2C_GetTiming(HAL_RCC_GetPCLK1Freq(), 100000U)` для 100 kHz на H743.

---

## Рекомендация

1. Взять за основу **NUCLEO-H723ZG → I2C_TwoBoards_ComPolling** (пины и логика совпадают с нашим Step 1).
2. Скопировать в проект `i2c_timing_utility.c` и `i2c_timing_utility.h`, в `MX_I2C1_Init()` подставлять:
   - `hi2c1.Init.Timing = I2C_GetTiming(HAL_RCC_GetPCLK1Freq(), 100000U);`
3. Для NUCLEO-H743ZI по-прежнему использовать **I2C1, PB8, PB9** (как в `main.h` примера H723ZG).

Так мы максимально приближаемся к готовому проверенному примеру I2C и избавляемся от ручного подбора TIMING при разных частотах.
