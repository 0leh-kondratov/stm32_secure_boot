# Gotowe przykłady I2C (STM32CubeH7 i NUCLEO-H743ZI)

## Gdzie szukać

W **STM32CubeH7** dla płyty **NUCLEO-H743ZI** nie ma osobnego folderu `Examples/I2C`. Najwygodniej jest bazować na przykładach dla **NUCLEO-H723ZG** – ten sam rząd H7, te same piny I2C1.

---

## 1. NUCLEO-H723ZG - I2C1, PB8/PB9 (kompatybilny z NUCLEO-H743ZI)

Ścieżka:
`STM32CubeH7/Projects/NUCLEO-H723ZG/Examples/I2C/`

| Przykład | Opis |
|--------|----------|
| **I2C_TwoBoards_ComPolling** | Master/Slave, odpytywanie. Inicjalizacja poprzez `HAL_I2C_Init`, obliczenie TIMINGu poprzez `I2C_GetTiming()`. |
| I2C_TwoBoards_ComIT | Przerwij wymianę zdań. |
| I2C_TwoBoards_ComDMA | Wymiana poprzez DMA. |
| I2C_WakeUpFromStop | Obudź się ze Stopu przez I2C. |

Piny (z readme):

- **SCL:** PB8 (CN7, D15)  
- **SDA:** PB9 (CN7, D14)  
- **Interfejs:** I2C1

Oznacza to, że ten sam PB8/PB9, co w naszym kroku 1 dla NUCLEO-H743ZI.

Kluczowe pliki:

- `Src/main.c` - wywołaj `HAL_I2C_Init`, `HAL_I2C_Master_Transmit` / `HAL_I2C_Master_Receive` (lub analogi Slave).
- `Src/stm32h7xx_hal_msp.c` - `HAL_I2C_MspInit`: GPIO (PB8/PB9, AF4), włącz taktowanie I2C1 i GPIOB.
- `Inc/main.h` - `I2Cx` (I2C1) makra, piny, porty, AF.
- `Src/i2c_timing_utility.c`, `Inc/i2c_timing_utility.h` — obliczenie rejestru **TIMING** na podstawie częstotliwości magistrali i PCLK (ważne dla stabilnej pracy na H7).

Przykład inicjalizacji I2C (fragment):

```c
I2cHandle.Instance            = I2C1;
I2cHandle.Init.Timing          = I2C_GetTiming(HAL_RCC_GetPCLK2Freq(), BUS_I2Cx_FREQUENCY);
I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_10BIT;
// ...inne pola
HAL_I2C_Init(&I2cHandle);
HAL_I2CEx_ConfigAnalogFilter(&I2cHandle, I2C_ANALOGFILTER_ENABLE);
```

Na **STM32H743** I2C1 zawiesza się na **APB1**, więc podczas migracji do H743 musisz użyć `HAL_RCC_GetPCLK1Freq()` zamiast `HAL_RCC_GetPCLK2Freq()` podczas wywoływania `I2C_GetTiming()`.

---

## 2. STM32H7B3I-EVAL – I2C i EEPROM

Ścieżka:
`STM32CubeH7/Projects/STM32H7B3I-EVAL/Examples/I2C/I2C_EEPROM_fast_mode_plus/`

- Wymiana z EEPROM M24LR64 poprzez I2C (Fast Mode Plus).
- Używane są stałe TIMING i DMA.
- Płytka i piny są różne, ale przydatne jako odniesienie do pracy z HAL I2C i synchronizacją.

---

## 3. Nasz krok 1 (stm32_secure_boot)

- **I2C:** I2C1, PB8 (SCL), PB9 (SDA) - jak w przykładzie H723ZG.
- W inicjalizacji `app/step1/main.c` poprzez `MX_I2C1_Init()` z **zakodowanym** na stałe** `Timing = 0x10909CEC` (100 kHz przy 200 MHz APB1).
- Jeśli po zmianie częstotliwości lub płyta I2C zachowuje się niestabilnie, warto przenieść narzędzie do obliczania taktowania z NUCLEO-H723ZG do projektu i wywołać:
- `I2C_GetTiming(HAL_RCC_GetPCLK1Freq(), 100000U)` dla 100 kHz na H743.

---

## Zalecenie

1. Jako podstawę weź **NUCLEO-H723ZG → I2C_TwoBoards_ComPolling** (piny i logika są takie same jak w naszym kroku 1).
2. Skopiuj `i2c_timing_utility.c` i `i2c_timing_utility.h` do projektu i podstaw w `MX_I2C1_Init()`:
   - `hi2c1.Init.Timing = I2C_GetTiming(HAL_RCC_GetPCLK1Freq(), 100000U);`
3. W przypadku NUCLEO-H743ZI nadal używaj **I2C1, PB8, PB9** (jak w `main.h` w przykładzie H723ZG).

W ten sposób maksymalnie zbliżamy się do gotowego, sprawdzonego przykładu I2C i pozbywamy się ręcznego doboru TIMINGU na różnych częstotliwościach.
