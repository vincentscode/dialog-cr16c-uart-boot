#include "commands.h"

#include <stddef.h>
#include <stdint.h>

#include "chipid.h"
#include "crc32.h"
#include "debug.h"
#if ENABLE_DISPLAY
#include "display.h"
#endif
#include "flash.h"
#include "protocol.h"
#include "system.h"
#include "uart.h"
#include "util.h"
#include "watchdog.h"

#define UART_CMD_PING         0x00
#define UART_CMD_SET_BAUDRATE 0x01
#define UART_CMD_FLASH_INFO   0x02
#define UART_CMD_ERASE_SECTOR 0x03
#define UART_CMD_PROGRAM_PAGE 0x04
#define UART_CMD_RESET        0x05
#define UART_CMD_READ_FLASH   0x06
#define UART_CMD_CHECKSUM     0x07
#define UART_CMD_CHIPID       0x08
#define UART_CMD_READMEM      0x09
#define UART_CMD_TESTING      0x10
#define UART_CMD_WRITE_FRAME  0x11

#define FLASH_PAGE_SIZE 256

struct cmd_handler {
    void (*call)(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len);
    unsigned int min_param_len;
    bool streaming;
};

static uint8_t flash_read_buffer[256];

static void call_ping_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    protocol_send_response(RESPONSE_OK, id);
}

static void call_set_baudrate_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    const uint32_t baudrate = read_le32(param_data);
    if (uart_is_baudrate_attainable(baudrate)) {
        protocol_send_response(RESPONSE_OK, id);
        uart_flush();
        uart_set_baudrate(baudrate);
    } else {
        protocol_send_response(RESPONSE_INVALID_PARAM, id);
    }
}

static void call_flash_info_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    uint8_t flash_info_buf[4];
    write_le32(flash_info_buf, flash_get_size());
    protocol_send_response_with_payload(RESPONSE_FLASH_INFO, id, flash_info_buf, sizeof(flash_info_buf));
}

static void call_erase_sector_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    const uint32_t address = read_le32(param_data);

    if (flash_erase_sector(address)) {
        protocol_send_response(RESPONSE_OK, id);
    } else {
        protocol_send_response(RESPONSE_FLASH_TIMEOUT, id);
    }
}

static void call_program_page_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    const uint8_t *param8 = param_data;
    const uint32_t address = read_le32(&param8[0]);

    if (flash_program_page(address, &param8[4])) {
        protocol_send_response(RESPONSE_OK, id);
    } else {
        protocol_send_response(RESPONSE_FLASH_TIMEOUT, id);
    }
}

static void call_reset_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    protocol_send_response(RESPONSE_OK, id);
    system_reset();
}

static void call_read_flash_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    const uint8_t *param8 = param_data;
    uint32_t start_address = read_le32(&param8[0]);
    uint32_t length = read_le32(&param8[4]);

    protocol_send_header(RESPONSE_OK, id, length);

    uint32_t crc = crc32_init();
    while (length) {
        uint32_t read_length = length;
        if (read_length > sizeof(flash_read_buffer)) {
            read_length = sizeof(flash_read_buffer);
        }

        flash_read(start_address, flash_read_buffer, read_length);
        crc = crc32_update(crc, flash_read_buffer, read_length);
        uart_write(flash_read_buffer, read_length);

        length -= read_length;
        start_address += read_length;
    }

    crc = crc32_final(crc);
    protocol_send_trailing_crc(crc);
}

static void call_checksum_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    const uint8_t *param8 = param_data;
    uint32_t start_address = read_le32(&param8[0]);
    uint32_t length = read_le32(&param8[4]);

    uint32_t crc = crc32_init();
    while (length) {
        uint32_t read_length = length;
        if (read_length > sizeof(flash_read_buffer)) {
            read_length = sizeof(flash_read_buffer);
        }

        flash_read(start_address, flash_read_buffer, read_length);
        crc = crc32_update(crc, flash_read_buffer, read_length);
        watchdog_reset();

        length -= read_length;
        start_address += read_length;
    }

    crc = crc32_final(crc);
    protocol_send_trailing_crc(crc);

    uint8_t crc_buf[4];
    write_le32(crc_buf, crc);
    protocol_send_response_with_payload(RESPONSE_CHECKSUM, id, crc_buf, sizeof(crc_buf));
}

static void call_chipid_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    chipid_t chipid;
    chipid_read(&chipid);
    const uint8_t chipid_buf[] = {
        chipid.id1,
        chipid.id2,
        chipid.id3,
        chipid.mem_size,
        chipid.revision
    };
    protocol_send_response_with_payload(RESPONSE_CHIPID, id, chipid_buf, sizeof(chipid_buf));
}

static void call_readn_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    const uint8_t *param8 = param_data;
    const uint32_t addr = read_le32(&param8[0]);
    const uint32_t len = read_le32(&param8[4]);
    protocol_send_response_with_payload(RESPONSE_OK, id, (void *) addr, len);
}

static void call_testing_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
    debug_puts("********\r\n");
    debug_puts("Hello World!\r\n");
    debug_puts("********\r\n");
    protocol_send_response(RESPONSE_OK, id);
}

#if ENABLE_DISPLAY
static void call_write_frame_handler(const cmd_handler_t *handler, const uint32_t id, const void *param_data, unsigned int param_len) {
    (void) param_data;
    uint32_t remaining = param_len;

    if (remaining >= 1) {
        const uint8_t num_segments = uart_stream_byte();
        remaining--;
        for (uint8_t s = 0; s < num_segments && remaining >= 5; s++) {
            const uint8_t y_start = uart_stream_byte();
            const uint8_t num_rows = uart_stream_byte();
            const uint8_t start_color = uart_stream_byte();
            uint32_t rle_len = (uint32_t) uart_stream_byte() | ((uint32_t) uart_stream_byte() << 8);
            remaining -= 5;
            if (rle_len > remaining) {
                rle_len = remaining;
            }
            display_write_band(y_start, num_rows, start_color, uart_stream_byte, rle_len);
            remaining -= rle_len;
        }
    }

    while (remaining--) {
        uart_stream_byte();
    }

    protocol_send_response(RESPONSE_OK, id);
}
#endif

static const cmd_handler_t cmd_handlers[] = {
    [UART_CMD_PING] = {
        .call = call_ping_handler,
        .min_param_len = 0,
    },
    [UART_CMD_SET_BAUDRATE] = {
        .call = call_set_baudrate_handler,
        .min_param_len = 4,
    },
    [UART_CMD_FLASH_INFO] = {
        .call = call_flash_info_handler,
        .min_param_len = 0,
    },
    [UART_CMD_ERASE_SECTOR] = {
        .call = call_erase_sector_handler,
        .min_param_len = 4,
    },
    [UART_CMD_PROGRAM_PAGE] = {
        .call = call_program_page_handler,
        .min_param_len = 4 + FLASH_PAGE_SIZE,
    },
    [UART_CMD_RESET] = {
        .call = call_reset_handler,
        .min_param_len = 0,
    },
    [UART_CMD_READ_FLASH] = {
        .call = call_read_flash_handler,
        .min_param_len = 8,
    },
    [UART_CMD_CHECKSUM] = {
        .call = call_checksum_handler,
        .min_param_len = 8,
    },
    [UART_CMD_CHIPID] = {
        .call = call_chipid_handler,
        .min_param_len = 0,
    },
    [UART_CMD_READMEM] = {
        .call = call_readn_handler,
        .min_param_len = 8,
    },
    [UART_CMD_TESTING] = {
        .call = call_testing_handler,
        .min_param_len = 0,
    },
#if ENABLE_DISPLAY
    [UART_CMD_WRITE_FRAME] = {
        .call = call_write_frame_handler,
        .min_param_len = 1,
        .streaming = true,
    },
#endif
};

const cmd_handler_t *command_lookup(const uint8_t cmd) {
    if (cmd >= ARRAY_SIZE(cmd_handlers) || cmd_handlers[cmd].call == NULL) {
        return NULL;
    }
    return &cmd_handlers[cmd];
}

unsigned int command_min_param_len(const cmd_handler_t *handler) {
    return handler->min_param_len;
}

bool command_is_streaming(const cmd_handler_t *handler) {
    return handler->streaming;
}

void command_dispatch(const cmd_handler_t *handler, const uint32_t id, const void *param_data, const unsigned int param_len) {
    handler->call(handler, id, param_data, param_len);
}
