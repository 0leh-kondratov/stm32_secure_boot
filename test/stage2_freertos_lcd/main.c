/**
 * Stage 2: FreeRTOS + LCD — "System rhythm" test.
 * One task updates I2C LCD 1602 with a counter every 1 second.
 * On startup: "Wallet Init" on line 1. Then line 1 = "Count: NNN", line 2 = "Wallet Init".
 * Checks: Systick (critical on H7), LCD inside RTOS task. No LwIP.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stm32h7xx_hal.h"
#include "uart_log.h"
#include "lcd_1602_i2c.h"

/* UART_LOG_USE_USART3 from Makefile */
I2C_HandleTypeDef hi2c1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void LCD_Task(void *pvParameters);
static void put_dec16(char *buf, uint16_t val);

/* SystemCoreClock defined in system_stm32h7xx.c; updated by SystemClock_Config */
extern uint32_t SystemCoreClock;

extern void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
  HAL_IncTick();
  xPortSysTickHandler();
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  log_init();
  log_puts("Stage 2: FreeRTOS + LCD\r\n");

  if (!LCD_1602_Init(&hi2c1))
  {
    log_puts("LCD init failed\r\n");
    for (;;) {}
  }
  LCD_1602_Print("Wallet Init", "...");
  HAL_Delay(500);

  xTaskCreate(LCD_Task, "LCD", 256, NULL, 1, NULL);
  vTaskStartScheduler();
  for (;;) {}
}

static void LCD_Task(void *pvParameters)
{
  uint16_t counter = 0;
  char line1[17];
  (void)pvParameters;

  for (;;)
  {
    line1[0] = 'C'; line1[1] = 'o'; line1[2] = 'u'; line1[3] = 'n'; line1[4] = 't'; line1[5] = ':'; line1[6] = ' ';
    put_dec16(line1 + 7, counter);
    LCD_1602_Print(line1, "Wallet Init");
    counter++;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void put_dec16(char *buf, uint16_t val)
{
  if (val >= 1000) { *buf++ = (char)('0' + val / 1000); val %= 1000; }
  if (val >= 100)  { *buf++ = (char)('0' + val / 100);  val %= 100; }
  if (val >= 10)   { *buf++ = (char)('0' + val / 10);   val %= 10; }
  *buf++ = (char)('0' + val);
  *buf = '\0';
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef o = {0};
  RCC_ClkInitTypeDef c = {0};
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

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
  hi2c1.Init.Timing = 0x10909CEC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) for (;;);
  HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
  HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}
