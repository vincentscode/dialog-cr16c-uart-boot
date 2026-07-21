#pragma once

#include <stdint.h>

#define ST7735_WIDTH   128
#define ST7735_HEIGHT  160

#define ST7735_COLOR_BLACK    0x0000
#define ST7735_COLOR_BLUE     0x001F
#define ST7735_COLOR_GREEN    0x07E0
#define ST7735_COLOR_CYAN     0x07FF
#define ST7735_COLOR_RED      0xF800
#define ST7735_COLOR_MAGENTA  0xF81F
#define ST7735_COLOR_YELLOW   0xFFE0
#define ST7735_COLOR_WHITE    0xFFFF

void display_init(void);

void display_fill(uint16_t color565);

void display_draw_test_pattern(void);

void display_backlight_on(void);
