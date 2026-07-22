#pragma once

#if defined(CHIP_SC14441)
    #define CHIP_NAME        "SC14441"
    #define CHIP_ID_DECIMAL  441
    // 32K SHARED_RAM (0x10000-0x17FFF)
    #define CHIP_RAM_ORIGIN  0x00010000
    #define CHIP_RAM_LENGTH  0x00008000
    #define CHIP_HAS_QSPI    1

#elif defined(CHIP_SC14481)
    #define CHIP_NAME        "SC14481"
    #define CHIP_ID_DECIMAL  481
    // 16K SHARED_RAM (0x10000-0x13FFF)
    #define CHIP_RAM_ORIGIN  0x00010000
    #define CHIP_RAM_LENGTH  0x00004000
    #define CHIP_HAS_QSPI    0

#elif defined(CHIP_SC14440)
    #define CHIP_NAME        "SC14440"
    #define CHIP_ID_DECIMAL  440
    // 16K SHARED_RAM (0x10000-0x13FFF) (this is just a guess)
    #define CHIP_RAM_ORIGIN  0x00010000
    #define CHIP_RAM_LENGTH  0x00004000
    #define CHIP_HAS_QSPI    0

#elif defined(CHIP_SC14444)
    #define CHIP_NAME        "SC14444"
    #define CHIP_ID_DECIMAL  444
    // 16K SHARED_RAM (0x10000-0x13FFF) (this is just a guess)
    #define CHIP_RAM_ORIGIN  0x00010000
    #define CHIP_RAM_LENGTH  0x00004000
    #define CHIP_HAS_QSPI    1

#elif defined(CHIP_SC14448)
    #define CHIP_NAME        "SC14448"
    #define CHIP_ID_DECIMAL  448
    // 16K SHARED_RAM (0x10000-0x13FFF) (this is just a guess)
    #define CHIP_RAM_ORIGIN  0x00010000
    #define CHIP_RAM_LENGTH  0x00004000
    #define CHIP_HAS_QSPI    1

#else
    #error "No target chip selected."
#endif

#define CHIP_RES_LENGTH  0x80
