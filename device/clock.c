#include "clock.h"

#include <stdint.h>

#include "gpio.h"
#include "power.h"

void clock_init(void) {
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
