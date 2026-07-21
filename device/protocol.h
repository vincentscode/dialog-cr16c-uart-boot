#pragma once

#include <stdint.h>

#define HEADER_BYTE            0xA5

#define RESPONSE_INVALID_CRC   0x00
#define RESPONSE_CMD_OK        0x01
#define RESPONSE_CMD_INVALID   0x02
#define RESPONSE_PARAM_SHORT   0x03
#define RESPONSE_OK            0x04
#define RESPONSE_DEBUG         0x05
#define RESPONSE_INVALID_PARAM 0x06
#define RESPONSE_ONLINE        0x07
#define RESPONSE_FLASH_TIMEOUT 0x08
#define RESPONSE_CHECKSUM      0x09
#define RESPONSE_FLASH_INFO    0x0A
#define RESPONSE_CHIPID        0x0B

void protocol_send_response(uint8_t response, uint32_t id);

void protocol_send_header(uint8_t response, uint32_t id, uint32_t payload_len);

void protocol_send_response_with_payload(uint8_t response, uint32_t id, const void *data, uint32_t len);

void protocol_send_trailing_crc(uint32_t crc);
