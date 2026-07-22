#pragma once

#include <stdint.h>

#define MMIO16(addr) (*((volatile uint16_t*)(addr)))
#define MMIO32(addr) (*((volatile uint32_t*)(addr)))
#define MMIO8(addr) (*((volatile uint8_t*)(addr)))

#define ARRAY_SIZE(x_) (sizeof(x_) / sizeof(*(x_)))

#define UNUSED1(x) (void)(x)
#define UNUSED2(x,y) (void)(x),(void)(y)
#define UNUSED3(x,y,z) (void)(x),(void)(y),(void)(z)
#define UNUSED4(a,x,y,z) (void)(a),(void)(x),(void)(y),(void)(z)
#define UNUSED5(a,b,x,y,z) (void)(a),(void)(b),(void)(x),(void)(y),(void)(z)

#define VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5, N,...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)

#define UNUSED_IMPL_(nargs) UNUSED ## nargs
#define UNUSED_IMPL(nargs) UNUSED_IMPL_(nargs)
#define UNUSED(...) UNUSED_IMPL( VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ )

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
