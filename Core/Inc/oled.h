#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* 适配常见 SSD1306 I2C 128x64 模块；7-bit 地址通常为 0x3C 或 0x3D。 */
#define OLED_WIDTH             128U
#define OLED_HEIGHT             64U
#define OLED_I2C_ADDR_7BIT    0x3CU
#define OLED_I2C_ADDR         (OLED_I2C_ADDR_7BIT << 1U)
#define OLED_I2C_TIMEOUT_MS   200U

HAL_StatusTypeDef OLED_Init(void);
HAL_StatusTypeDef OLED_Update(void);
void OLED_Clear(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t on);
void OLED_DrawChar(uint8_t x, uint8_t y, char ch);
void OLED_DrawString(uint8_t x, uint8_t y, const char *text);

#endif
