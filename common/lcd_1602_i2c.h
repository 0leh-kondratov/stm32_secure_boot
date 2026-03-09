/**
 * I2C LCD 1602 driver (PCF8574 at 0x27).
 * Use a Mutex to protect I2C access when other tasks use the same bus.
 */
#ifndef LCD_1602_I2C_H
#define LCD_1602_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Default I2C address for PCF8574-based LCD 1602 */
#define LCD_1602_I2C_ADDR    (0x27U << 1)

/**
 * Initialize LCD 1602 on I2C (address 0x27).
 * hi2c: HAL I2C handle (e.g. &hi2c1). I2C must be initialized before.
 * Returns true on success.
 * Use a Mutex to protect I2C access if other tasks use the same bus.
 */
bool LCD_1602_Init(void *hi2c);

/**
 * Print string to line 1 or 2 (1-based). Line is 16 chars; string is truncated.
 */
void LCD_1602_SetLine1(const char *str);
void LCD_1602_SetLine2(const char *str);

/**
 * Clear display and home cursor.
 */
void LCD_1602_Clear(void);

/**
 * Single API to set both lines (convenience).
 */
void LCD_1602_Print(const char *line1, const char *line2);

#ifdef __cplusplus
}
#endif

#endif /* LCD_1602_I2C_H */
