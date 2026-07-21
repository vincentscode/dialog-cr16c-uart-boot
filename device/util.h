#pragma once

#include <stdint.h>

#define MMIO16(addr) (*((volatile uint16_t*)(addr)))
#define MMIO32(addr) (*((volatile uint32_t*)(addr)))
#define MMIO8(addr) (*((volatile uint8_t*)(addr)))

#define ARRAY_SIZE(x_) (sizeof(x_) / sizeof(*(x_)))

typedef void (*funcp_t)(void);

static inline uint32_t read_le32(const void *data) {
    const uint8_t *data8 = data;
    return (uint32_t) data8[0] |
           (uint32_t) data8[1] << 8 |
           (uint32_t) data8[2] << 16 |
           (uint32_t) data8[3] << 24;
}

static inline void write_le32(void *data, const uint32_t val) {
    uint8_t *data8 = data;
    data8[0] = (val >> 0) & 0xff;
    data8[1] = (val >> 8) & 0xff;
    data8[2] = (val >> 16) & 0xff;
    data8[3] = (val >> 24) & 0xff;
}
