#ifndef __OLED_H
#define __OLED_H

#include "stm32g0xx_hal.h"
#include <stdint.h>

/* SSD1315 I2C address: 7-bit = 0x3C (0111100b). HAL_I2C_Master_Transmit
   expects the address left-shifted by 1, so pass 0x3C << 1 = 0x78. */
#define OLED_I2C_ADDR   (0x3C << 1)

extern const uint8_t font5x7[95][5]; /* ASCII 0x20-0x7E, 5 bytes per glyph */

HAL_StatusTypeDef OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(uint8_t pattern);
void OLED_SetCursor(uint8_t page, uint8_t col);
void OLED_ClearLine(uint8_t page);
void OLED_WriteChar(uint8_t page, uint8_t col, char c);
void OLED_WriteString(uint8_t page, uint8_t col, const char *str);

#endif /* __OLED_H */
