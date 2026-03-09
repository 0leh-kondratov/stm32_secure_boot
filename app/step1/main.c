/**
 * Step 1 minimal: только вывод в UART. Без I2C, без LCD.
 * Как в demo — минимум кода, только log_init() и log_puts().
 */
#include <stdint.h>
#include "stm32h7xx_hal.h"
#include "uart_log.h"

static void SystemClock_Config(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  log_init();

  log_puts("\r\nStep1 UART OK\r\n");

  for (;;)
  {
    log_puts(".\r\n");
    HAL_Delay(1000);
  }
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

void SysTick_Handler(void)
{
  HAL_IncTick();
}
