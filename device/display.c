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

#define ST7735_CMD_RAMWR 0x2C

#define ST7735_SLPOUT  0x11
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_DISPON  0x29

#define LCD_CMD(cmd, ...) do {                           \
    static const uint8_t data_[] = { __VA_ARGS__ };      \
    lcd_write_cmd_and_data((cmd), data_, sizeof(data_)); \
} while (0)

static void lcd_reset(void) {
    LCD_RST_HIGH();
    delay_ms(20);
    LCD_RST_LOW();
    delay_ms(200);
    LCD_RST_HIGH();
    delay_ms(120);
}

static void lcd_init(void) {
    LCD_CS_LOW();

    lcd_reset();

    lcd_write_cmd_and_data(ST7735_SLPOUT, 0, 0);
    delay_ms(120);

    LCD_CMD(ST7735_FRMCTR1, 0x05, 0x3C, 0x3C);
    LCD_CMD(ST7735_FRMCTR2, 0x05, 0x3C, 0x3C);
    LCD_CMD(ST7735_FRMCTR3, 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C);
    LCD_CMD(ST7735_INVCTR,  0x07);

    LCD_CMD(ST7735_PWCTR1, 0x0E, 0x0E, 0x04);
    LCD_CMD(ST7735_PWCTR2, 0xC0);
    LCD_CMD(ST7735_PWCTR3, 0x0D, 0x00);
    LCD_CMD(ST7735_PWCTR4, 0x8B, 0x2A);
    LCD_CMD(ST7735_PWCTR5, 0x8D, 0xEE);
    LCD_CMD(ST7735_VMCTR1, 0x06);

    LCD_CMD(ST7735_MADCTL, 0x10);

    LCD_CMD(ST7735_GMCTRP1, 0x0E, 0x1D, 0x11, 0x11, 0x29, 0x24, 0x1F, 0x23, 0x23, 0x21, 0x2A, 0x37, 0x00, 0x09, 0x02, 0x2B);
    LCD_CMD(ST7735_GMCTRN1, 0x0E, 0x1C, 0x0F, 0x0A, 0x29, 0x24, 0x1E, 0x22, 0x22, 0x21, 0x2A, 0x37, 0x00, 0x09, 0x01, 0x2B);

    LCD_CMD(ST7735_COLMOD, 0x05);
    LCD_CMD(ST7735_CASET, 0x00, 0x02, 0x00, 0x81);
    LCD_CMD(ST7735_RASET, 0x00, 0x01, 0x00, 0xA0);

    lcd_write_cmd_and_data(ST7735_DISPON, 0, 0);

    LCD_CS_HIGH();
}

void display_init(void) {
    lcd_gpio_init();
    clock_init();
    spi_init();
    lcd_init();
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
    CP_CTRL_REG = CP_CTRL_REG_CP_EN(3) | CP_CTRL_REG_CP_LEVEL1(3) | CP_CTRL_REG_CP_LEVEL2(3) | CP_CTRL_REG_CP_FREQ(1);
    LED_CTRL_REG = LED_CTRL_REG_WLED_CUR0(3);
}
