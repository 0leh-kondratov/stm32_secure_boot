/**
 * I2C LCD 1602 (PCF8574 at 0x27) — 4-bit mode, HAL I2C.
 * PCF8574: P0-P3 = D4-D7, P4 = RS, P5 = RW, P6 = E, P7 = Backlight.
 * Caller (e.g. lcd_monitor_task) must hold I2C mutex before any LCD_1602_* call.
 */
#include "lcd_1602_i2c.h"
#include <string.h>

#ifdef USE_HAL_DRIVER
#include "stm32h7xx_hal.h"
#endif

static I2C_HandleTypeDef *s_hi2c;
static uint8_t s_lcd_addr = LCD_1602_I2C_ADDR;

/* PCF8574: write 8-bit value (data nibbles + control). */
static void lcd_write_nibble(uint8_t nibble, uint8_t rs)
{
  uint8_t v = (nibble & 0x0FU) | (rs ? 0x10U : 0x00U) | 0x08U; /* E high, backlight */
  if (s_hi2c) {
    HAL_I2C_Master_Transmit(s_hi2c, s_lcd_addr, &v, 1, 10);
    v &= ~0x08U;
    HAL_I2C_Master_Transmit(s_hi2c, s_lcd_addr, &v, 1, 10);
  }
}

static void lcd_send_byte(uint8_t byte, uint8_t rs)
{
  lcd_write_nibble(byte >> 4, rs);
  lcd_write_nibble(byte & 0x0FU, rs);
}

static void lcd_cmd(uint8_t cmd)
{
  lcd_send_byte(cmd, 0);
}

static void lcd_data(uint8_t data)
{
  lcd_send_byte(data, 1);
}

bool LCD_1602_Init(void *hi2c)
{
  if (hi2c == NULL) return false;
  s_hi2c = (I2C_HandleTypeDef *)hi2c;
  HAL_Delay(40);
  /* 4-bit init sequence */
  lcd_write_nibble(0x03, 0);
  HAL_Delay(5);
  lcd_write_nibble(0x03, 0);
  HAL_Delay(1);
  lcd_write_nibble(0x03, 0);
  lcd_write_nibble(0x02, 0); /* 4-bit mode */
  lcd_cmd(0x28);             /* 2 lines, 5x8 */
  lcd_cmd(0x0C);             /* display on, no cursor */
  lcd_cmd(0x06);             /* entry mode */
  LCD_1602_Clear();
  return true;
}

void LCD_1602_Clear(void)
{
  lcd_cmd(0x01);
  HAL_Delay(2);
}

void LCD_1602_SetLine1(const char *str)
{
  lcd_cmd(0x80); /* DDRAM 0 */
  for (int i = 0; i < 16; i++) {
    if (str && str[i]) lcd_data((uint8_t)str[i]);
    else lcd_data(' ');
  }
}

void LCD_1602_SetLine2(const char *str)
{
  lcd_cmd(0xC0); /* DDRAM 0x40 */
  for (int i = 0; i < 16; i++) {
    if (str && str[i]) lcd_data((uint8_t)str[i]);
    else lcd_data(' ');
  }
}

void LCD_1602_Print(const char *line1, const char *line2)
{
  LCD_1602_SetLine1(line1 ? line1 : "");
  LCD_1602_SetLine2(line2 ? line2 : "");
}
