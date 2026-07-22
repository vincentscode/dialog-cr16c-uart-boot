#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chip_config.h"
#include "chipid.h"
#include "commands.h"
#include "crc32.h"
#include "debug.h"
#include "display.h"
#include "flash.h"
#include "irq.h"
#include "protocol.h"
#include "qspi.h"
#include "system.h"
#include "traps.h"
#include "uart.h"
#include "util.h"
#include "watchdog.h"

#define TIMEOUT_HEADER 0x1000000
#define TIMEOUT_CMD    0x10000
#define TIMEOUT_PARAM  0x100000

typedef enum cmd_state {
    CMD_STATE_WAIT_HEADER,
    CMD_STATE_WAIT_CMD,
    CMD_STATE_WAIT_PARAM
} cmd_state_t;

vector_table_t vector_table = {
    .svc_trap = svc_trap,
    .dvz_trap = dvz_trap,
    .flg_trap = flg_trap,
    .bpt_trap = bpt_trap,
    .trc_trap = trc_trap,
    .und_trap = und_trap,
    .iad_trap = iad_trap,
    .dbg_trap = dbg_trap,
    .uart_ri_int = uart_rx_int,
};

static void chip_id_guard(void) {
    chipid_t chip_id;
    chipid_read(&chip_id);
    const unsigned int combined_chip_id = (chip_id.id1 - '0') * 100 + (chip_id.id2 - '0') * 10 + (chip_id.id3 - '0');

    if (combined_chip_id != CHIP_ID_DECIMAL) {
        debug_puts("WARNING: loader built for " CHIP_NAME " but running on a different chip\r\n");
    }
}

int main(void) {
    uart_init();

    // empty debug to avoid first actual debug out being consumed by python frontend lol
    debug_puts("");

    chip_id_guard();

#if CHIP_HAS_QSPI
    qspi_init();
#endif

    watchdog_reset();

    asm("excp svc");
    debug_puts("SVC call returned\r\n");

#if CHIP_HAS_QSPI
    flash_detect();
#endif

#if ENABLE_DISPLAY
    display_init();
    display_draw_test_pattern();
    display_backlight_on();
    debug_puts("Display initialized\r\n");
#endif

    for (int i = 0; i < 3; i++) {
        protocol_send_response(RESPONSE_ONLINE, 0xFFFFFFFF);
    }

    cmd_state_t cmd_state = CMD_STATE_WAIT_HEADER;
    uint32_t parameter_len;
    uint32_t id;
    const cmd_handler_t *current_handler;
    unsigned long timeout = TIMEOUT_HEADER;
    while (1) {
        if (cmd_state == CMD_STATE_WAIT_HEADER) {
            if (uart_data_available()) {
                uint8_t datum = uart_read_byte();
                if (datum == HEADER_BYTE) {
                    cmd_state = CMD_STATE_WAIT_CMD;
                    timeout = TIMEOUT_CMD;
                }
            }
        }

        if (cmd_state == CMD_STATE_WAIT_CMD) {
            unsigned int data_len = uart_rx_buffered_data();
            if (data_len >= 13) {
                uint8_t *hdr = uart_get_read_ptr();
                uart_advance_read_ptr(13);
                uint8_t cmd = hdr[0];
                id = read_le32(&hdr[1]);
                parameter_len = read_le32(&hdr[5]);
                uint32_t crc_check = crc32_init();
                crc_check = crc32_update(crc_check, hdr, 9);
                crc_check = crc32_final(crc_check);
                uint32_t crc = read_le32(&hdr[9]);

                if (crc_check == crc) {
                    current_handler = command_lookup(cmd);
                    if (current_handler) {
                        if (parameter_len >= command_min_param_len(current_handler)) {
                            if (command_is_streaming(current_handler)) {
                                command_dispatch(current_handler, id, NULL, parameter_len);
                                uart_rx_dma_reset();
                                timeout = TIMEOUT_HEADER;
                                cmd_state = CMD_STATE_WAIT_HEADER;
                            } else {
                                timeout = TIMEOUT_PARAM;
                                cmd_state = CMD_STATE_WAIT_PARAM;
                            }
                        } else {
                            timeout = TIMEOUT_HEADER;
                            cmd_state = CMD_STATE_WAIT_HEADER;
                            protocol_send_response(RESPONSE_PARAM_SHORT, id);
                            uart_rx_dma_reset();
                        }
                    } else {
                        timeout = TIMEOUT_HEADER;
                        cmd_state = CMD_STATE_WAIT_HEADER;
                        protocol_send_response(RESPONSE_CMD_INVALID, id);
                        uart_rx_dma_reset();
                    }
                } else {
                    debug_puts("Invalid CRC32 on header ");
                    debug_hexdump(hdr, 13);
                    debug_puts("\r\n");
                    debug_puts("Expected 0x");
                    debug_putlong_hex(crc_check);
                    debug_puts(" but received 0x");
                    debug_putlong_hex(crc);
                    debug_puts("\r\n");
                    timeout = TIMEOUT_HEADER;
                    cmd_state = CMD_STATE_WAIT_HEADER;
                    protocol_send_response(RESPONSE_INVALID_CRC, id);
                    uart_rx_dma_reset();
                }
            }
        }

        if (cmd_state == CMD_STATE_WAIT_PARAM) {
            if (parameter_len) {
                unsigned int data_len = uart_rx_buffered_data();
                if (data_len >= parameter_len + 4) {
                    const uint8_t *read_ptr = uart_get_read_ptr();
                    uart_advance_read_ptr(parameter_len + 4);
                    uint32_t crc_check = crc32_init();
                    crc_check = crc32_update(crc_check, read_ptr, parameter_len);
                    crc_check = crc32_final(crc_check);
                    uint32_t crc = read_le32(&read_ptr[parameter_len]);
                    if (crc_check == crc) {
                        command_dispatch(current_handler, id, read_ptr, parameter_len);
                        uart_rx_dma_reset();
                        cmd_state = CMD_STATE_WAIT_HEADER;
                        timeout = TIMEOUT_HEADER;
                    } else {
                        debug_puts("Invalid CRC32 on params ");
                        debug_hexdump(read_ptr, parameter_len + 4);
                        debug_puts("\r\n");
                        debug_puts("Expected 0x");
                        debug_putlong_hex(crc_check);
                        debug_puts(" but received 0x");
                        debug_putlong_hex(crc);
                        debug_puts("\r\n");
                        timeout = TIMEOUT_HEADER;
                        cmd_state = CMD_STATE_WAIT_HEADER;
                        protocol_send_response(RESPONSE_INVALID_CRC, id);
                        uart_rx_dma_reset();
                    }
                }
            } else {
                command_dispatch(current_handler, id, NULL, 0);
                uart_rx_dma_reset();
                cmd_state = CMD_STATE_WAIT_HEADER;
                timeout = TIMEOUT_HEADER;
            }
        }

        if (timeout) {
            timeout--;
            watchdog_reset();
        } else {
            debug_puts("Timeout elapsed, resetting\r\n");
            system_reset();
        }
    }
}
