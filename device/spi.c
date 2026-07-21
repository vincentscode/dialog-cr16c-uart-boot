#include "spi.h"

#include <stdbool.h>
#include <stdint.h>

#include "dma.h"
#include "gpio.h"
#include "watchdog.h"

#define SPI0_WAIT_TIMEOUT 50000UL
#define SPI0_DMA_TIMEOUT  0x800000UL

static void spi0_pins_and_controller_bringup(void) {
    gpio_set_mode(GPIO(0, 5), 1);
    gpio_set_mode(GPIO(0, 6), 1);
    gpio_set_output(GPIO(0, 5));
    gpio_set_output(GPIO(0, 6));

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

void spi_init(void) {
    spi0_pins_and_controller_bringup();
    spi0_configure_mode0_byte();
}

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

void spi_tx_byte(uint8_t b) {
    spi0_wait_tx_holding_empty();
    SPI_RX_TX_REG0 = b;
    spi0_wait_idle();
}

void spi_set_word16(bool enable16) {
    SPI_CTRL_REG &= ~0x0001;
    SPI_CTRL_REG = (uint16_t) ((SPI_CTRL_REG & ~SPI_CTRL_REG_WORD_MASK) | (enable16 ? SPI_CTRL_REG_WORD_16 : 0));
    SPI_CTRL_REG |= 0x0001;
}

void spi_blit16_dma(const uint16_t *src, uint32_t count, bool src_increment) {
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
