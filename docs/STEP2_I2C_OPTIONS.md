# Opcje inicjalizacji I2C (krok 2, NUCLEO-H743ZI)

## Porównanie z NUCLEO-F207ZG (Krótkie dane / Nucleo-144)

Według [Data brief Nucleo-144] (https://www.st.com/resource/en/data_brief/nukleo-f207zg.pdf), płyty **NUCLEO-F207ZG** i **NUCLEO-H743ZI** wchodzą w skład tej samej linii (Nucleo-144), a w przypadku niektórych płytek używana jest **ta sama instrukcja obsługi UM1974** (odniesienie do płyty **MB1137**).

### Gdzie jest I2C na płycie

- **Na złączu CN8** (lewa dolna część płytki) **I2C nie jest domyślnie włączone**.
- **I2C jest wyprowadzane na złącze CN7** (kompatybilne z ST Zio / Arduino):

| Sygnał | Przypnij Zio | Fizyczny pin MCU | Lokalizacja na pokładzie |
|--------|---------|---------------------|------------------------|
| **SCL** | D15 | **PB8** | CN7, pin 2 (prawy górny róg) |
| **SDA** | D14 | **PB9** | CN7, pin 4 (drugi od góry po prawej) |

Musisz podłączyć moduł I2C (LCD itp.) do **CN7**, a nie do CN8.

### Mapowanie do kroku 2

| Parametr | NUCLEO-F207ZG | NUCLEO-H743ZI (krok 2) |
|----------|----------------|------------------------|
| **MK** | STM32F207ZGT6 (Cortex-M3, 120 MHz) | STM32H743ZIT6 (Cortex-M7, 400 MHz) |
| **I2C do CN7** | I2C1: **PB8 (SCL), PB9 (SDA)** | To samo z `I2Cx_USE_PB8_PB9 1` |
| **Alternatywa (Morpo)** | — | PB6 (SCL), PB7 (SDA) z `I2Cx_USE_PB8_PB9 0` |
| **AF dla I2C1** | AF4 (GPIOB) | AF4 (GPIOB) - mecze |
| **Inicjalizacja I2C** | HAL STM32F2xx (inne API) | HAL STM32H7xx (rejestr I2C_Timing) |

**Wyjście:** Aby okablowanie pokrywało się z **NUCLEO-F207ZG** (złącze **CN7**), w kroku 2 ustaw `#define I2Cx_USE_PB8_PB9 1` - następnie SCL = PB8 (D15), SDA = PB9 (D14) na CN7.

---

## Bieżąca konfiguracja

- **Magistrala:** I2C1
- **Piny:** PB6 = SCL, PB7 = SDA (AF4)
- **Poprawki:** włączone wewnętrzne GPIO_PULLUP (`I2Cx_GPIO_PULLUP 1` w `main.h`)
- **Przed inicjacją:** urządzenia peryferyjne I2C1 są resetowane (w przypadku zamrożenia magistrali)
- **Prędkość:** Taktowanie `0x10909CEC` (~100 kHz przy 200 MHz APB1; przy 100 MHz APB1 — ~50 kHz)
- **Filtry:** analogowe włączone, cyfrowe 0

---

## Opcje, które można zmienić

### 1. Szelki SDA/SCL

| Opcja | Gdzie zmienić | Kiedy używać |
|--------|------------|---------------------|
| **Szelki wewnętrzne (teraz)** | `main.h`: `#define I2Cx_GPIO_PULLUP 1` | Jeżeli na magistrali nie ma rezystorów zewnętrznych (moduł bez podciągania i długich przewodów). |
| **Brak szelek** | `main.h`: `#define I2Cx_GPIO_PULLUP 0` | Jeżeli moduł/szyna ma już rezystory od 2,2–4,7 kΩ do 3,3 V. |

Wiele wyświetlaczy LCD z PCF8574 ma już własne podciągnięcia - wtedy możesz spróbować `I2Cx_GPIO_PULLUP 0`. Jeśli „Nie znaleziono urządzenia I2C” - zwróć 1.

### 2. Piny I2C1 (alternatywa dla PB6/PB7)

W przełączniku `app/step2/Inc/main.h` **`I2Cx_USE_PB8_PB9`**:

| Znaczenie | Szpilki | Użycie |
|----------|------|----------------|
| **0** (domyślnie) | PB6 SCL, PB7 SDA | Morpho (w CN8 I2C nie jest domyślnie włączone). |
| **1** | PB8 SCL, PB9 SDA | **CN7** (Zio): D15=PB8 SCL, D14=PB9 SDA - tak samo jak w NUCLEO-F207ZG. |

Zmień opcję: wstaw `#define I2Cx_USE_PB8_PB9 1` w `main.h` i odbuduj.

### 3. Zresetuj I2C przed inicjalizacją

Teraz w `MX_I2C1_Init()` przed konfiguracją wykonywane są następujące czynności:
`__HAL_RCC_I2C1_FORCE_RESET()` / `RELEASE_RESET()`.

Pomaga to, jeśli po twardym resecie lub zamarznięciu opona pozostaje w nieprawidłowym stanie. Jeśli pojawią się dziwne usterki, możesz tymczasowo usunąć tę blokadę i sprawdzić.

### 4. Prędkość (czas)

- **Bieżąca wartość:** `0x10909CEC` - tryb standardowy (~100 kHz lub mniej przy 100 MHz APB1).
- W przypadku trybu szybkiego (400 kHz) potrzebujesz innej wartości taktowania (na przykład z CubeMX lub [dokumentacja ST](https://www.st.com/resource/en/application_note/an4235-i2c-timing-configuration-tool-for-stm32-microcontrollers-stmicroelectronics.pdf)). W przypadku skanera i PCF8574 lepiej pozostawić 100 kHz.

### 5. Limit czasu i liczba prób skanowania

`do_i2c_scan()` używa:
- `HAL_I2C_IsDeviceReady(..., 2, 50)` — 2 próby, timeout 50 ms.

Jeśli magistrala jest powolna lub hałaśliwa, możesz ją zwiększyć: 3–5 prób, limit czasu 100 ms.

### 6. Testowanie bez urządzeń na magistrali

„Nie znaleziono urządzenia I2C” jest normalne, jeśli nic nie jest podłączone do PB6/PB7.
Jeśli moduł jest podłączony (np. LCD z PCF8574) i nadal nie ma urządzeń:

1. Sprawdź okablowanie: SCL → PB6, SDA → PB7, GND, VCC (3,3 V).
2. Spróbuj zamienić SDA i SCL po stronie modułu.
3. Włącz wewnętrzne podciąganie (`I2Cx_GPIO_PULLUP 1`).
4. Upewnij się, że moduł jest włączony i kontrast LCD jest ustawiony (niebieskie paski bez tekstu często oznaczają zasilanie bez inicjalizacji przez I2C).

---

## Szybkie sprawdzenie

1. Zbuduj i flashuj krok 2: `zrób krok 2 i& wykonaj flash-krok 2`.
2. Otwórz UART 115200 - Bloki „Krok 2: OK” i „Skanowanie I2C...” powinny pojawiać się co 5 sekund.
3. Podłącz wyświetlacz LCD (GND, VCC, SDA → PB7, SCL → PB6), zresetuj płytkę - w logu powinien pojawić się komunikat `[FOUND] 0x27` lub `0x3F`.
