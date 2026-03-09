/**
 * Stage 1: I2C Scanner — "Live hardware" test.
 * No FreeRTOS. Scans I2C bus (0x03..0x77) with HAL_I2C_IsDeviceReady,
 * prints found addresses to UART3 (115200). NUCLEO-H743ZI: I2C1 PB6/PB7.
 * Checks: wiring, 5V power, I2C timings. Expected: 0x27 (PCF8574 LCD) or 0x3F.
 */
#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "uart_log.h"

/* UART_LOG_USE_USART3 set by Makefile */
I2C_HandleTypeDef hi2c1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void put_hex8(uint8_t v);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  log_init();
  log_puts("\r\n=== Stage 1: I2C Scanner ===\r\n");
  log_puts("Scanning I2C1 (PB6 SCL, PB7 SDA)...\r\n");

  int found = 0;
  for (uint8_t addr = 0x03; addr <= 0x77; addr++)
  {
    uint8_t dev_addr = addr << 1;
    if (HAL_I2C_IsDeviceReady(&hi2c1, dev_addr, 2, 50) == HAL_OK)
    {
      log_puts("  [FOUND] 0x");
      put_hex8(addr);
      log_puts(" (7-bit)\r\n");
      found++;
    }
  }

  if (found == 0)
    log_puts("  No device found. Check wires and 5V.\r\n");
  else
  {
    log_puts("Total: ");
    if (found >= 10) { put_hex8((uint8_t)(found / 10)); }
    put_hex8((uint8_t)(found % 10));
    log_puts(" device(s). LCD 1602 is often 0x27 or 0x3F.\r\n");
  }

  log_puts("Done. Loop.\r\n");
  for (;;)
    HAL_Delay(1000);
}

static void put_hex8(uint8_t v)
{
  const char hex[] = "0123456789ABCDEF";
  char s[3] = { hex[v >> 4], hex[v & 0x0F], '\0' };
  log_puts(s);
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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
}

static void MX_I2C1_Init(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_I2C1_CLK_ENABLE();
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  g.Mode = GPIO_MODE_AF_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &g);

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10909CEC; /* 100 kHz @ 200 MHz APB1 */
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) for (;;);
  (void)HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
  (void)HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}
