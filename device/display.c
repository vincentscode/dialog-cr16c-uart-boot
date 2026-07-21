#include "display.h"

#include <stdbool.h>
#include <stdint.h>

#include "dma.h"
#include "gpio.h"
#include "util.h"
#include "watchdog.h"

#define SPI_CTRL_REG           MMIO16(0xFF4940)
#define SPI_CTRL_REG_TXH       (1 << 11)
#define SPI_CTRL_REG_WORD_MASK (3 << 7)
#define SPI_CTRL_REG_WORD_16   (1 << 7)
#define SPI_RX_TX_REG0         MMIO16(0xFF4942)
#define SPI_CLEAR_INT_REG      MMIO16(0xFF4946)
#define SPI_CTRL_REG1          MMIO16(0xFF4948)
#define SPI_CTRL_REG1_BUSY     (1 << 3)

#define CLK_AMBA_REG        MMIO16(0xFF4000)
#define CLK_CODEC_DIV_REG   MMIO16(0xFF4002)
#define CLK_PER_DIV_REG     MMIO16(0xFF400C)
#define CLK_PER10_DIV_REG   MMIO16(0xFF400E)
#define CLK_PLL_CTRL_REG    MMIO16(0xFF4010)
#define CLK_PLL_DIV_REG     MMIO16(0xFF4012)
#define CLK_AUX_REG         MMIO16(0xFF401A)
#define CLK_ULP_CTRL_REG    MMIO16(0xFF401E)

#define BAT_CTRL_REG        MMIO16(0xFF4812)
#define BAT_CTRL2_REG       MMIO16(0xFF4814)
#define CP_CTRL_REG         MMIO16(0xFF481C)
#define PAD_CTRL_REG        MMIO16(0xFF481E)
#define LED_CTRL_REG        MMIO16(0xFF4820)

static void pll_and_clock_bringup(void) {
    CLK_ULP_CTRL_REG = 0;
    BAT_CTRL_REG = 0x45;

    CLK_PER10_DIV_REG = 0x181;
    CLK_PER_DIV_REG = 9;
    CLK_CODEC_DIV_REG = 200;
    CLK_PLL_DIV_REG = 0x1C;
    CLK_PLL_CTRL_REG = 0x852;

    for (volatile unsigned int i = 0xFFF; i != 0; i--) {}

    CLK_AMBA_REG |= 0x0020;
    CLK_AMBA_REG |= 0x0080;
    CLK_AMBA_REG &= 0xFFE7;
    CLK_AMBA_REG = (uint16_t) ((CLK_AMBA_REG & 0xFFF8) | 0x0001);

    CLK_PLL_CTRL_REG |= 0x0008;
    BAT_CTRL2_REG = 0x862;

    CP_CTRL_REG = 0x35;
    CLK_AUX_REG = 0x420;
    P1_DATA_REG &= 0xFFDF;
    PAD_CTRL_REG = 0x3C0;
}

static void spi0_pin_and_clock_bringup(void) {
    pll_and_clock_bringup();

    P0_MODE_REG = (uint16_t) ((P0_MODE_REG & 0xFCFF) | 0x0100);
    P0_MODE_REG = (uint16_t) ((P0_MODE_REG & 0xF3FF) | 0x0400);
    P0_DIR_REG |= 0x0C00;
    P0_DIR_REG |= 0x3000;

    SPI_CTRL_REG &= 0xFFFE;
    SPI_CTRL_REG |= 0x0002;
    SPI_CTRL_REG |= 0x0004;
    SPI_CTRL_REG |= 0x0020;
    SPI_CTRL_REG &= 0xFBFF;
    SPI_CTRL_REG &= 0xFFBF;
    SPI_CTRL_REG &= 0xFE7F;
    SPI_CTRL_REG = (uint16_t) ((SPI_CTRL_REG & 0xFFE7) | 0x0010);
    SPI_CTRL_REG &= 0xFDFF;
    SPI_CTRL_REG |= 0x4000;
    SPI_CTRL_REG |= 0x0001;
}

static void spi0_configure_mode0_byte(void) {
    SPI_CTRL_REG &= ~0x0001;
    SPI_CTRL_REG &= ~0x0002; // CPHA = 0
    SPI_CTRL_REG &= ~0x0004; // CPOL = 0
    SPI_CTRL_REG = (uint16_t) ((SPI_CTRL_REG & ~0x0018) | ((2 << 3) & 0x0018));
    SPI_CTRL_REG &= ~0x0180;
    SPI_CTRL_REG &= ~0x4000;
    SPI_CTRL_REG1 = (uint16_t) ((SPI_CTRL_REG1 & ~0x0003) | 0x0002);
    SPI_CTRL_REG |= 0x0001;
}

#define SPI0_WAIT_TIMEOUT 50000UL

static void spi0_tx_byte(uint8_t b) {
    uint32_t timeout = SPI0_WAIT_TIMEOUT;
    while (SPI_CTRL_REG & SPI_CTRL_REG_TXH) {
        watchdog_reset();
        if (--timeout == 0) {
            return;
        }
    }
    SPI_RX_TX_REG0 = b;
    timeout = SPI0_WAIT_TIMEOUT;
    while (SPI_CTRL_REG1 & SPI_CTRL_REG1_BUSY) {
        watchdog_reset();
        if (--timeout == 0) {
            return;
        }
    }
}

#define SPI0_DMA_TIMEOUT 0x800000UL

static void spi0_wait_tx_holding_empty(void) {
    uint32_t timeout = SPI0_WAIT_TIMEOUT;
    while (SPI_CTRL_REG & SPI_CTRL_REG_TXH) {
        watchdog_reset();
        if (--timeout == 0) {
            return;
        }
    }
}

static void spi0_wait_idle(void) {
    uint32_t timeout = SPI0_WAIT_TIMEOUT;
    while (SPI_CTRL_REG1 & SPI_CTRL_REG1_BUSY) {
        watchdog_reset();
        if (--timeout == 0) {
            return;
        }
    }
}

static void spi0_set_word16(bool enable16) {
    SPI_CTRL_REG &= ~0x0001;
    SPI_CTRL_REG = (uint16_t) ((SPI_CTRL_REG & ~SPI_CTRL_REG_WORD_MASK) | (enable16 ? SPI_CTRL_REG_WORD_16 : 0));
    SPI_CTRL_REG |= 0x0001;
}

static void spi0_blit16_dma(const uint16_t *src, uint32_t count, bool src_increment) {
    if (count == 0) {
        return;
    }

    spi0_wait_tx_holding_empty();
    SPI_CLEAR_INT_REG = 1;
    SPI_RX_TX_REG0 = src[0];

    if (count == 1) {
        spi0_wait_idle();
        return;
    }

    const uint32_t dma_count = count - 1;
    const uint16_t *dma_src = src_increment ? &src[1] : &src[0];

    DMAX_A_STARTL_REG(DMA_SPI_TX) = (uint16_t) (uintptr_t) dma_src;
    DMAX_A_STARTH_REG(DMA_SPI_TX) = (uint16_t) ((uint32_t) (uintptr_t) dma_src >> 16);
    DMAX_B_STARTL_REG(DMA_SPI_TX) = (uint16_t) (uintptr_t) &SPI_RX_TX_REG0;
    DMAX_B_STARTH_REG(DMA_SPI_TX) = (uint16_t) ((uint32_t) (uintptr_t) &SPI_RX_TX_REG0 >> 16);
    DMAX_INT_REG(DMA_SPI_TX) = (uint16_t) dma_count;
    DMAX_LEN_REG(DMA_SPI_TX) = (uint16_t) dma_count;
    DMAX_CTRL_REG(DMA_SPI_TX) = DMAX_CTRL_REG_DMA_PRIO_LOWMID | (src_increment ? DMAX_CTRL_REG_AINC : 0) | DMAX_CTRL_REG_DREQ_MODE | DMAX_CTRL_REG_BW_HALFWORD;
    DMAX_CTRL_REG(DMA_SPI_TX) |= DMAX_CTRL_REG_DMA_ON;

    uint32_t timeout = SPI0_DMA_TIMEOUT;
    while (timeout--) {
        watchdog_reset();
        if (!(DMAX_CTRL_REG(DMA_SPI_TX) & DMAX_CTRL_REG_DMA_ON)) {
            break;
        }
        if (DMAX_IDX_REG(DMA_SPI_TX) >= dma_count) {
            break;
        }
    }

    DMAX_CTRL_REG(DMA_SPI_TX) &= ~DMAX_CTRL_REG_DMA_ON;
    spi0_wait_idle();
}

#define LCD_CS_BIT   4
#define LCD_DC_BIT   2
#define LCD_RST_BIT  2
#define LCD_CS_PIN   (1 << LCD_CS_BIT)
#define LCD_DC_PIN   (1 << LCD_DC_BIT)
#define LCD_RST_PIN  (1 << LCD_RST_BIT)
#define LCD_CS_DIR   (0x3u << (2 * LCD_CS_BIT))   // TODO: make generic P0.4
#define LCD_DC_DIR   (0x3u << (2 * LCD_DC_BIT))   // TODO: make generic P2.2
#define LCD_RST_DIR  (0x3u << (2 * LCD_RST_BIT))  // TODO: make generic P3.2

#define LCD_CS_LOW()   (P0_RESET_DATA_REG = LCD_CS_PIN)
#define LCD_CS_HIGH()  (P0_SET_DATA_REG   = LCD_CS_PIN)
#define LCD_DC_CMD()   (P2_RESET_DATA_REG = LCD_DC_PIN)
#define LCD_DC_DATA()  (P2_SET_DATA_REG   = LCD_DC_PIN)
#define LCD_RST_LOW()  (P3_RESET_DATA_REG = LCD_RST_PIN)
#define LCD_RST_HIGH() (P3_SET_DATA_REG   = LCD_RST_PIN)

static void lcd_gpio_init(void) {
    P0_DIR_REG |= LCD_CS_DIR;
    P2_DIR_REG |= LCD_DC_DIR;
    P3_DIR_REG |= LCD_RST_DIR;

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
    spi0_tx_byte(cmd);
    if (len) {
        LCD_DC_DATA();
        while (len--) {
            spi0_tx_byte(*data++);
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
    spi0_pin_and_clock_bringup();
    spi0_configure_mode0_byte();
    lcd_run_init_table(st7735_init_table);
}

void display_ramwr_begin(void) {
    LCD_DC_CMD();
    LCD_CS_LOW();
    spi0_tx_byte(ST7735_CMD_RAMWR);
    LCD_DC_DATA();
    spi0_set_word16(true);
}

void display_write_pixels(const uint16_t *pixels, uint32_t count) {
    spi0_blit16_dma(pixels, count, true);
}

void display_ramwr_end(void) {
    spi0_set_word16(false);
    LCD_CS_HIGH();
}

void display_fill(uint16_t color565) {
    static uint16_t color;
    color = color565;

    display_ramwr_begin();
    spi0_blit16_dma(&color, (uint32_t) ST7735_WIDTH * ST7735_HEIGHT, false);
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
