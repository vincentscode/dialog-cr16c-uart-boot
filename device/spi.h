#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

#define SPI_CTRL_REG           MMIO16(0xFF4940)
#define SPI_CTRL_REG_TXH       (1 << 11)
#define SPI_CTRL_REG_WORD_MASK (3 << 7)
#define SPI_CTRL_REG_WORD_16   (1 << 7)
#define SPI_RX_TX_REG0         MMIO16(0xFF4942)
#define SPI_CLEAR_INT_REG      MMIO16(0xFF4946)
#define SPI_CTRL_REG1          MMIO16(0xFF4948)
#define SPI_CTRL_REG1_BUSY     (1 << 3)

void spi_init(void);

void spi_tx_byte(uint8_t b);

void spi_set_word16(bool enable16);

void spi_blit16_dma(const uint16_t *src, uint32_t count, bool src_increment);
