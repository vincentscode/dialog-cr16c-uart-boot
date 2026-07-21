#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

#define P0_DATA_REG MMIO16(0xFF4830)
#define P0_SET_DATA_REG MMIO16(0xFF4832)
#define P0_RESET_DATA_REG MMIO16(0xFF4834)
#define P0_DIR_REG MMIO16(0xFF4836)
#define P0_MODE_REG MMIO16(0xFF4838)

#define P1_DATA_REG MMIO16(0xFF4840)
#define P1_SET_DATA_REG MMIO16(0xFF4842)
#define P1_RESET_DATA_REG MMIO16(0xFF4844)
#define P1_DIR_REG MMIO16(0xFF4846)
#define P1_MODE_REG MMIO16(0xFF4848)

#define P2_DATA_REG MMIO16(0xFF4850)
#define P2_SET_DATA_REG MMIO16(0xFF4852)
#define P2_RESET_DATA_REG MMIO16(0xFF4854)
#define P2_DIR_REG MMIO16(0xFF4856)
#define P2_MODE_REG MMIO16(0xFF4858)

#define P3_DATA_REG MMIO16(0xFF4860)
#define P3_SET_DATA_REG MMIO16(0xFF4862)
#define P3_RESET_DATA_REG MMIO16(0xFF4864)
#define P3_DIR_REG MMIO16(0xFF4866)
#define P3_MODE_REG MMIO16(0xFF4868)

#define P4_DATA_REG MMIO16(0xFF4870)
#define P4_SET_DATA_REG MMIO16(0xFF4872)
#define P4_RESET_DATA_REG MMIO16(0xFF4874)
#define P4_DIR_REG MMIO16(0xFF4876)
#define P4_MODE_REG MMIO16(0xFF4878)

#define P5_DATA_REG MMIO16(0xFF4880)
#define P5_SET_DATA_REG MMIO16(0xFF4882)
#define P5_RESET_DATA_REG MMIO16(0xFF4884)
#define P5_DIR_REG MMIO16(0xFF4886)
#define P5_MODE_REG MMIO16(0xFF4888)

#define PORT_BASE(i)         (0xFF4830L + ((unsigned long)i) * 0x10L)
#define PORT_DATA_REG(port)  MMIO16(PORT_BASE(port) + 0x00)
#define PORT_SET_REG(port)   MMIO16(PORT_BASE(port) + 0x02)
#define PORT_RESET_REG(port) MMIO16(PORT_BASE(port) + 0x04)
#define PORT_DIR_REG(port)   MMIO16(PORT_BASE(port) + 0x06)
#define PORT_MODE_REG(port)  MMIO16(PORT_BASE(port) + 0x08)

typedef uint8_t gpio_t;

#define GPIO(port, pin) ((gpio_t) (((port) << 4) | ((pin) & 0x0F)))
#define GPIO_PORT(g)    ((unsigned int) ((g) >> 4))
#define GPIO_PIN(g)     ((unsigned int) ((g) & 0x0F))

#define GPIO_DIR_OUTPUT 0x3u

static inline void gpio_set_output(gpio_t g) {
    PORT_DIR_REG(GPIO_PORT(g)) |= (uint16_t) (GPIO_DIR_OUTPUT << (2 * GPIO_PIN(g)));
}

// probably SC14441 specific
static const uint8_t gpio_mode_field[6][8] = {
    // P0: 1-bit P0.0/P0.1, 2-bit P0.2-7
    {0x10, 0x11, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C},
    // P1: 1-bit P1.0-4, 3-bit P1.5, none on P1.6/P1.7
    {0x10, 0x11, 0x12, 0x13, 0x14, 0x35, 0x00, 0x00},
    // P2: 2-bit P2.0-5, 1-bit P2.6@13 / P2.7@15 (INT6/7 between)
    {0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x1D, 0x1F},
    // P3: uniform 2-bit
    {0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E},
    // P4: 1-bit P4.2@0 and P4.1@1 only
    {0x00, 0x11, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00},
    // P5: single 5-bit port field, mapped to pin 0
    {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static inline void gpio_set_mode(gpio_t g, unsigned int mode) {
    const uint8_t field = gpio_mode_field[GPIO_PORT(g)][GPIO_PIN(g)];
    const unsigned int width = (unsigned int) (field >> 4);
    if (width == 0) {
        return;
    }
    const unsigned int shift = field & 0x0Fu;
    const unsigned int mask = (1u << width) - 1u;
    PORT_MODE_REG(GPIO_PORT(g)) = (uint16_t) ((PORT_MODE_REG(GPIO_PORT(g)) & ~(mask << shift)) | ((mode & mask) << shift));
}

static inline void gpio_set_mode_bits(gpio_t g, unsigned int bits) {
    const uint8_t field = gpio_mode_field[GPIO_PORT(g)][GPIO_PIN(g)];
    const unsigned int width = (unsigned int) (field >> 4);
    if (width == 0) {
        return;
    }
    const unsigned int shift = field & 0x0Fu;
    const unsigned int mask = (1u << width) - 1u;
    PORT_MODE_REG(GPIO_PORT(g)) |= (uint16_t) ((bits & mask) << shift);
}

static inline void gpio_set_high(gpio_t g) {
    PORT_SET_REG(GPIO_PORT(g)) = (uint16_t) (1u << GPIO_PIN(g));
}

static inline void gpio_set_low(gpio_t g) {
    PORT_RESET_REG(GPIO_PORT(g)) = (uint16_t) (1u << GPIO_PIN(g));
}

static inline void gpio_write(gpio_t g, bool high) {
    if (high) {
        gpio_set_high(g);
    } else {
        gpio_set_low(g);
    }
}

static inline bool gpio_read(gpio_t g) {
    return (PORT_DATA_REG(GPIO_PORT(g)) >> GPIO_PIN(g)) & 1u;
}
