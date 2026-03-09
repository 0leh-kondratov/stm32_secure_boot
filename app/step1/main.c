/**
 * Step 1: Hardware Baseline — I2C2 (PF1/PF0) scanner + LCD 1602 (PCF8574 @ 0x27).
 * No FreeRTOS. HAL from STM32CubeH7.
 * Verification: "Device Found at 0x27" on UART; "Ready" on LCD.
 */
#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "uart_log.h"
#include "lcd_1602_i2c.h"

I2C_HandleTypeDef hi2c2;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);

/** Scan I2C bus (0x03..0x77). Returns true if device at 7-bit addr is present. */
static bool I2C_Scan_IsDevicePresent(uint8_t addr_7bit)
{
  uint8_t dev = (uint8_t)(addr_7bit << 1);
  return (HAL_I2C_IsDeviceReady(&hi2c2, dev, 2, 50) == HAL_OK);
}

/** Scan and print found addresses to UART. Returns true if 0x27 found. */
static bool I2C_Scan(void)
{
  log_puts("I2C2 (PF1 SCL, PF0 SDA) scan...\r\n");
  bool found_27 = false;
  for (uint8_t addr = 0x03; addr <= 0x77; addr++)
  {
    if (I2C_Scan_IsDevicePresent(addr))
    {
      if (addr == 0x27)
      {
        log_puts("Device Found at 0x27\r\n");
        found_27 = true;
      }
      else
      {
        const char hex[] = "0123456789ABCDEF";
        log_puts("  [0x");
        char s[3] = { hex[addr >> 4], hex[addr & 0x0F], '\0' };
        log_puts(s);
        log_puts("]\r\n");
      }
    }
  }
  if (!found_27)
    log_puts("0x27 not found (check PCF8574 wiring)\r\n");
  return found_27;
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C2_Init();

  log_init();
  log_puts("\r\n=== Step 1: I2C + LCD Baseline ===\r\n");

  if (!I2C_Scan())
  {
    log_puts("Abort: PCF8574 at 0x27 required.\r\n");
    for (;;) HAL_Delay(1000);
  }

  if (!LCD_1602_Init(&hi2c2))
  {
    log_puts("LCD_Init failed.\r\n");
    for (;;) HAL_Delay(1000);
  }
  LCD_1602_Print("Ready", "");
  log_puts("LCD: Ready\r\n");

  for (;;)
    HAL_Delay(1000);
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef o = {0};
  RCC_ClkInitTypeDef c = {0};

  o.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  o.HSEState = RCC_HSE_BYPASS;
  o.PLL.PLLState = RCC_PLL_ON;
  o.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  o.PLL.PLLM = 4;
  o.PLL.PLLN = 400;
  o.PLL.PLLP = 2;
  o.PLL.PLLQ = 4;
  o.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&o) != HAL_OK) for (;;);

  c.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1;
  c.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  c.SYSCLKDivider = RCC_SYSCLK_DIV1;
  c.AHBCLKDivider = RCC_HCLK_DIV2;
  c.APB3CLKDivider = RCC_APB3_DIV2;
  c.APB1CLKDivider = RCC_APB1_DIV2;
  c.APB2CLKDivider = RCC_APB2_DIV2;
  c.APB4CLKDivider = RCC_APB4_DIV2;
  if (HAL_RCC_ClockConfig(&c, FLASH_LATENCY_4) != HAL_OK) for (;;);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
}

/** I2C2 on PF1 (SCL), PF0 (SDA) — CN8 labels I2C2_SCL/I2C2_SDA on NUCLEO-H743ZI. */
static void MX_I2C2_Init(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_I2C2_CLK_ENABLE();
  g.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  g.Mode = GPIO_MODE_AF_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOF, &g);

  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x10909CEC; /* 100 kHz @ 200 MHz APB1 */
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK) for (;;);
  (void)HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE);
  (void)HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0);
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}
