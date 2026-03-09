# Step 1 minimal: только UART

Минимальный образ — только вывод в UART. Без I2C, без LCD.

- **UART:** USART2 (PA2/PA3), 115200 8N1 — виртуальный COM порт ST-Link.
- После сброса: строка `Step1 UART OK`, затем раз в секунду `.\r\n`.

## Сборка и прошивка

```bash
make step1
make flash-step1
```

Открыть терминал на COM-порте ST-Link, 115200 8N1, нажать Reset на плате.
