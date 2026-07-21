#include "protocol.h"

#include <stdint.h>

#include "crc32.h"
#include "uart.h"
#include "util.h"

void protocol_send_header(const uint8_t response, const uint32_t id, const uint32_t payload_len) {
    uint8_t hdr[14];
    hdr[0] = HEADER_BYTE;
    hdr[1] = response;
    write_le32(&hdr[2], id);
    write_le32(&hdr[6], payload_len);
    uint32_t crc = crc32_init();
    crc = crc32_update(crc, hdr, 10);
    crc = crc32_final(crc);
    write_le32(&hdr[10], crc);
    uart_write(hdr, sizeof(hdr));
}

void protocol_send_trailing_crc(const uint32_t crc) {
    uint8_t crc_buf[4];
    write_le32(crc_buf, crc);
    uart_write(crc_buf, sizeof(crc_buf));
}

void protocol_send_response_with_payload(const uint8_t response, const uint32_t id, const void *data, const uint32_t len) {
    protocol_send_header(response, id, len);
    uint32_t crc = crc32_init();
    crc = crc32_update(crc, data, len);
    crc = crc32_final(crc);
    uart_write(data, len);
    protocol_send_trailing_crc(crc);
}

void protocol_send_response(const uint8_t response, const uint32_t id) {
    protocol_send_header(response, id, 0);
}
