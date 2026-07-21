#include "debug.h"

#include <stdint.h>
#include <string.h>

#include "crc32.h"
#include "protocol.h"
#include "uart.h"
#include "watchdog.h"

#define DEBUG_ID 0xFFFFFFFF

#define NIBBLE_TO_HEX_CHAR(i) ((i) < 10 ? '0' + (i) : 'A' + ((i) - 10))

void debug_puts(const char *ptr) {
    protocol_send_response_with_payload(RESPONSE_DEBUG, DEBUG_ID, ptr, strlen(ptr));
}

void debug_putint(unsigned int val) {
    char str[6];
    str[4] = '0';
    str[5] = 0;
    const char *ptr = &str[4];
    for (int i = 4; i >= 0; i--) {
        if (!val) {
            break;
        }
        str[i] = '0' + (val % 10);
        ptr = &str[i];
        val /= 10;
    }
    debug_puts(ptr);
}

void debug_putlong(unsigned long val) {
    char str[11];
    str[9] = '0';
    str[10] = 0;
    const char *ptr = &str[9];
    for (int i = 9; i >= 0; i--) {
        if (!val) {
            break;
        }
        str[i] = '0' + (val % 10);
        ptr = &str[i];
        val /= 10;
    }
    debug_puts(ptr);
}

void debug_putbyte_hex(const unsigned char byt) {
    char str[3];
    str[0] = NIBBLE_TO_HEX_CHAR((byt >> 4) & 0xf);
    str[1] = NIBBLE_TO_HEX_CHAR(byt & 0xf);
    str[2] = 0;
    debug_puts(str);
}

void debug_putint_hex(const unsigned int i) {
    debug_putbyte_hex(i >> 8);
    debug_putbyte_hex(i & 0xff);
}

void debug_putlong_hex(const unsigned long i) {
    debug_putint_hex(i >> 16);
    debug_putint_hex(i & 0xffff);
}

void debug_hexdump(const void *ptr, unsigned int len) {
    protocol_send_header(RESPONSE_DEBUG, DEBUG_ID, len * 2);
    const uint8_t *ptr8 = ptr;
    uint32_t crc = crc32_init();
    while (len--) {
        const uint8_t byt = *ptr8++;
        const char hexdata[2] = {
            NIBBLE_TO_HEX_CHAR((byt >> 4) & 0xf),
            NIBBLE_TO_HEX_CHAR((byt >> 0) & 0xf),
        };
        crc = crc32_update(crc, hexdata, sizeof(hexdata));
        uart_write(hexdata, sizeof(hexdata));
        watchdog_reset();
    }
    crc = crc32_final(crc);
    protocol_send_trailing_crc(crc);
}
