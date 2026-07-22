#pragma once

#include "util.h"

#define CLK_PER10_DIV_REG               MMIO16(0xFF400E)
#define CLK_PER10_DIV_REG_QSPI_DIV_MASK (3 << 8)
#define CLK_PER10_DIV_REG_QSPI_DIV_1    (1 << 8)
#define CLK_PER10_DIV_REG_QSPI_DIV_2    (2 << 8)
#define CLK_PER10_DIV_REG_QSPI_EN       (1 << 10)

#define CLK_AMBA_REG               MMIO16(0xFF4000)
#define CLK_AMBA_REG_HCLK_DIV_MASK (7 << 0)
#define CLK_AMBA_REG_HCLK_DIV_8    (0 << 0)
#define CLK_AMBA_REG_HCLK_DIV_1    (1 << 0)
#define CLK_AMBA_REG_HCLK_DIV_2    (2 << 0)
#define CLK_AMBA_REG_HCLK_DIV_3    (3 << 0)
#define CLK_AMBA_REG_HCLK_DIV_4    (4 << 0)
#define CLK_AMBA_REG_HCLK_DIV_5    (5 << 0)
#define CLK_AMBA_REG_HCLK_DIV_6    (6 << 0)
#define CLK_AMBA_REG_HCLK_DIV_7    (7 << 0)
#define CLK_AMBA_REG_PCLK_DIV_MASK (3 << 3)
#define CLK_AMBA_REG_PCLK_DIV_4    (0 << 3)
#define CLK_AMBA_REG_PCLK_DIV_1    (1 << 3)
#define CLK_AMBA_REG_PCLK_DIV_2    (2 << 3)
#define CLK_AMBA_REG_SRAM1_EN      (1 << 6)

#define CLK_CODEC_DIV_REG MMIO16(0xFF4002)
#define CLK_PER_DIV_REG   MMIO16(0xFF400C)
#define CLK_PLL_CTRL_REG  MMIO16(0xFF4010)
#define CLK_PLL_DIV_REG   MMIO16(0xFF4012)
#define CLK_AUX_REG       MMIO16(0xFF401A)
#define CLK_ULP_CTRL_REG  MMIO16(0xFF401E)

void clock_init(void);
