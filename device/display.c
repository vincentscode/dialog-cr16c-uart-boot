#include "display.h"

#include <stdint.h>

#include "clock.h"
#include "gpio.h"
#include "power.h"
#include "spi.h"
#include "watchdog.h"

#define LCD_CS   GPIO(0, 4)
#define LCD_DC   GPIO(2, 2)
#define LCD_RST  GPIO(3, 2)

#define LCD_CS_LOW()   gpio_set_low(LCD_CS)
#define LCD_CS_HIGH()  gpio_set_high(LCD_CS)
#define LCD_DC_CMD()   gpio_set_low(LCD_DC)
#define LCD_DC_DATA()  gpio_set_high(LCD_DC)
#define LCD_RST_LOW()  gpio_set_low(LCD_RST)
#define LCD_RST_HIGH() gpio_set_high(LCD_RST)

static void lcd_gpio_init(void) {
    gpio_set_output(LCD_CS);
    gpio_set_output(LCD_DC);
    gpio_set_output(LCD_RST);

    LCD_CS_HIGH();
    LCD_DC_DATA();
    LCD_RST_HIGH();
}

static void delay_ms(unsigned int ms) {
    while (ms--) {
        watchdog_reset();
        for (volatile unsigned int i = 0; i < 6000; i++) {}
    }
}

static void lcd_write_cmd_and_data(uint8_t cmd, const uint8_t *data, unsigned int len) {
    LCD_DC_CMD();
    spi_tx_byte(cmd);
    if (len) {
        LCD_DC_DATA();
        while (len--) {
            spi_tx_byte(*data++);
        }
    }
}

// copied from default firmware init table
static const uint8_t st7735_init_table[] = {
    0xFB, 0xC8, 0x78,
    0xFA, 0x00, 0x11,
    0xF9, 0x78,
    0xFA, 0x03, 0xB1, 0x05, 0x3C, 0x3C,
    0xFA, 0x03, 0xB2, 0x05, 0x3C, 0x3C,
    0xFA, 0x06, 0xB3, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C,
    0xFA, 0x01, 0xB4, 0x07,
    0xFA, 0x03, 0xC0, 0x0E, 0x0E, 0x04,
    0xFA, 0x01, 0xC1, 0xC0,
    0xFA, 0x02, 0xC2, 0x0D, 0x00,
    0xFA, 0x02, 0xC3, 0x8B, 0x2A,
    0xFA, 0x02, 0xC4, 0x8D, 0xEE,
    0xFA, 0x01, 0xC5, 0x06,
    0xFA, 0x01, 0x36, 0x10,
    0xFA, 0x10, 0xE0, 0x0E, 0x1D, 0x11, 0x11, 0x29, 0x24, 0x1F, 0x23, 0x23, 0x21, 0x2A, 0x37, 0x00, 0x09, 0x02, 0x2B,
    0xFA, 0x10, 0xE1, 0x0E, 0x1C, 0x0F, 0x0A, 0x29, 0x24, 0x1E, 0x22, 0x22, 0x21, 0x2A, 0x37, 0x00, 0x09, 0x01, 0x2B,
    0xFA, 0x01, 0x3A, 0x05,
    0xFA, 0x04, 0x2A, 0x00, 0x02, 0x00, 0x81,
    0xFA, 0x04, 0x2B, 0x00, 0x01, 0x00, 0xA0,
    0xFA, 0x00, 0x29,
    0xFF,
};

#define ST7735_CMD_RAMWR 0x2C

static void lcd_run_init_table(const uint8_t *p) {
    LCD_CS_LOW();
    while (1) {
        const uint8_t op = *p++;
        if (op == 0xFA) {
            const uint8_t n = *p++;
            const uint8_t cmd = *p++;
            lcd_write_cmd_and_data(cmd, p, n);
            p += n;
        } else if (op == 0xF9) {
            delay_ms(*p++);
        } else if (op == 0xFB) {
            const uint8_t hold_ms = p[0];
            const uint8_t settle_ms = p[1];
            p += 2;
            LCD_RST_HIGH();
            delay_ms(20);
            LCD_RST_LOW();
            delay_ms(hold_ms);
            LCD_RST_HIGH();
            delay_ms(settle_ms);
        } else {
            break;
        }
    }
    LCD_CS_HIGH();
}

void display_init(void) {
    lcd_gpio_init();
    clock_init();
    spi_init();
    lcd_run_init_table(st7735_init_table);
}

void display_ramwr_begin(void) {
    LCD_DC_CMD();
    LCD_CS_LOW();
    spi_tx_byte(ST7735_CMD_RAMWR);
    LCD_DC_DATA();
    spi_set_word16(true);
}

void display_write_pixels(const uint16_t *pixels, uint32_t count) {
    spi_blit16_dma(pixels, count, true);
}

void display_ramwr_end(void) {
    spi_set_word16(false);
    LCD_CS_HIGH();
}

void display_fill(uint16_t color565) {
    static uint16_t color;
    color = color565;

    display_ramwr_begin();
    spi_blit16_dma(&color, (uint32_t) ST7735_WIDTH * ST7735_HEIGHT, false);
    display_ramwr_end();
}

void display_draw_test_pattern(void) {
    static const uint16_t bars[8] = {
        ST7735_COLOR_RED,
        ST7735_COLOR_GREEN,
        ST7735_COLOR_BLUE,
        ST7735_COLOR_YELLOW,
        ST7735_COLOR_CYAN,
        ST7735_COLOR_MAGENTA,
        ST7735_COLOR_WHITE,
        ST7735_COLOR_BLACK
    };
    static uint16_t line[ST7735_WIDTH];

    display_ramwr_begin();
    for (unsigned int y = 0; y < ST7735_HEIGHT; y++) {
        for (unsigned int x = 0; x < ST7735_WIDTH; x++) {
            if (x == 0 || x == ST7735_WIDTH - 1 || y == 0 || y == ST7735_HEIGHT - 1) {
                line[x] = ST7735_COLOR_RED;
            } else {
                line[x] = bars[x >> 4];
            }
        }
        display_write_pixels(line, ST7735_WIDTH);
    }
    display_ramwr_end();
}

void display_backlight_on(void) {
    CP_CTRL_REG = 0x013F;  // CP_EN=11, CP_LEVEL1=11, CP_LEVEL2=11
    LED_CTRL_REG = 0x0003; // WLED_CUR0=11
}
