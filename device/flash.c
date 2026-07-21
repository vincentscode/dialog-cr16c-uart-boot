#include "flash.h"

#include <stdint.h>

#include "debug.h"
#include "qspi.h"
#include "util.h"

#define JEDEC_CMD_RDID   0x9F
#define JEDEC_CMD_RDSFDP 0x5A
#define JEDEC_CMD_WREN   0x06
#define JEDEC_CMD_WRDI   0x04
#define JEDEC_CMD_RDSR   0x05
#define JEDEC_RDSR_WIP   (1 << 0)
#define JEDEC_RDSR_WEL   (1 << 1)

#define JEDEC_CMD_READ         0x03
#define JEDEC_CMD_PAGE_PROGRAM 0x02
#define JEDEC_CMD_SECTOR_ERASE 0x20

#define FLASH_PAGE_SIZE 256

#define TIMEOUT_SECTOR_ERASE 0x4000
#define TIMEOUT_PAGE_PROGRAM 0x400

typedef struct jedec_nor_flash_sector {
    uint8_t erase_opcode;
    uint8_t size_exponent;
} jedec_nor_flash_sector_t;

typedef struct jedec_nor_flash_info {
    uint32_t size_bytes;
    uint8_t erase_opcode_4kib;
    jedec_nor_flash_sector_t erase_sector_types[4];
} jedec_nor_flash_info_t;

static jedec_nor_flash_info_t flash_info_g = {0};

static const uint8_t sfdp_read_header_cmd[] = {JEDEC_CMD_RDSFDP, 0x00, 0x00, 0x00};

static void qspic_read_erase_sector_size(const uint8_t sector_desc[2], jedec_nor_flash_sector_t *sector_info) {
    sector_info->size_exponent = sector_desc[0];
    sector_info->erase_opcode = sector_desc[1];
    if (sector_info->size_exponent != 0xff) {
        debug_puts("Sector size 2^");
        debug_putint(sector_info->size_exponent);
        debug_puts(" erase opcode is 0x");
        debug_putbyte_hex(sector_info->erase_opcode);
        debug_puts("\r\n");
    }
}

static void qspic_read_parameter_table_0(uint8_t parameter_table[64], jedec_nor_flash_info_t *flash_info) {
    flash_info->erase_opcode_4kib = parameter_table[2];
    debug_puts("4KiB erase opcode 0x");
    debug_putbyte_hex(flash_info->erase_opcode_4kib);
    debug_puts("\r\n");

    const uint32_t raw_size = (uint32_t) parameter_table[4] | (uint32_t) parameter_table[5] << 8 | (uint32_t) parameter_table[6] << 16 | (uint32_t) parameter_table[7] << 24;

    debug_puts("Raw flash size 0x");
    debug_putlong_hex(raw_size);
    debug_puts("\r\n");
    if (raw_size & (1L << 31)) {
        flash_info->size_bytes = (1 << raw_size) / 8UL;
    } else {
        flash_info->size_bytes = (raw_size + 1) / 8UL;
    }

    for (int i = 0; i < 4; i++) {
        qspic_read_erase_sector_size(&parameter_table[7 * 4 + i / 2], &flash_info->erase_sector_types[i]);
    }
}

static void qspic_read_parameter_table(const uint8_t *parameter_header, jedec_nor_flash_info_t *flash_info) {
    uint8_t parameter_table[64];
    const unsigned int table_len = (unsigned int) parameter_header[3] * 4;
    if (table_len > sizeof(parameter_table)) {
        debug_puts("Parameter table > 64 byte, skipping");
        return;
    }
    uint8_t read_parameter_table_cmd[] = {
        JEDEC_CMD_RDSFDP, parameter_header[6], parameter_header[5], parameter_header[4]
    };
    const qspi_xfer_desc_t read_param_table_desc = {
        .mode = QSPI_MODE_SIO,
        .tx_data = read_parameter_table_cmd,
        .tx_len = sizeof(read_parameter_table_cmd),
        .dummy_cycles_after_tx = 1,
        .rx_data = parameter_table,
        .rx_len = table_len
    };
    qspi_write_then_read(&read_param_table_desc);
    const unsigned int table_type = parameter_header[0];
    debug_puts("Parameter table @0x");
    debug_putbyte_hex(parameter_header[6]);
    debug_putbyte_hex(parameter_header[5]);
    debug_putbyte_hex(parameter_header[4]);
    debug_puts(": ");
    debug_hexdump(parameter_table, table_len);
    debug_puts("\r\n");

    if (table_type == 0x00) {
        qspic_read_parameter_table_0(parameter_table, flash_info);
    }
}

static void qspic_read_sfdp(jedec_nor_flash_info_t *flash_info) {
    uint8_t sfdp_header[8];
    const qspi_xfer_desc_t desc = {
        .mode = QSPI_MODE_SIO,
        .tx_data = sfdp_read_header_cmd,
        .tx_len = sizeof(sfdp_read_header_cmd),
        .dummy_cycles_after_tx = 1,
        .rx_data = sfdp_header,
        .rx_len = sizeof(sfdp_header)
    };
    qspi_write_then_read(&desc);
    debug_puts("SFDP header: ");
    debug_hexdump(sfdp_header, sizeof(sfdp_header));
    debug_puts("\r\n");

    const unsigned int num_parameter_header = sfdp_header[6] + 1;
    debug_puts("Found ");
    debug_putint(num_parameter_header);
    debug_puts(" parameter headers\r\n");

    if (num_parameter_header == 0x100) {
        debug_puts("Flash does not seem to support SFDP, skipping auto detection\r\n");
        return;
    }

    for (unsigned int hdr_idx = 0; hdr_idx < num_parameter_header; hdr_idx++) {
        uint8_t parameter_header[8];

        const uint32_t address = sizeof(sfdp_header) + sizeof(parameter_header) * hdr_idx;
        uint8_t read_parameter_header_cmd[] = {
            JEDEC_CMD_RDSFDP, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
        };
        qspi_xfer_desc_t read_param_header_desc = {
            .mode = QSPI_MODE_SIO,
            .tx_data = read_parameter_header_cmd,
            .tx_len = sizeof(read_parameter_header_cmd),
            .dummy_cycles_after_tx = 1,
            .rx_data = parameter_header,
            .rx_len = sizeof(parameter_header)
        };
        qspi_write_then_read(&read_param_header_desc);
        debug_puts("Parameter header ");
        debug_putint(hdr_idx);
        debug_puts(": ");
        debug_hexdump(parameter_header, sizeof(parameter_header));
        debug_puts("\r\n");
        qspic_read_parameter_table(parameter_header, flash_info);
    }
}

static uint8_t read_flash_status_register(void) {
    const uint8_t read_status_register_cmd[] = {JEDEC_CMD_RDSR};
    uint8_t status_register[1];
    const qspi_xfer_desc_t read_status_register_desc = {
        .mode = QSPI_MODE_SIO,
        .tx_data = read_status_register_cmd,
        .tx_len = sizeof(read_status_register_cmd),
        .dummy_cycles_after_tx = 0,
        .rx_data = status_register,
        .rx_len = sizeof(status_register)
    };
    qspi_write_then_read(&read_status_register_desc);

    return status_register[0];
}

static bool wait_flash_write_finished(unsigned int timeout) {
    do {
        const uint8_t status = read_flash_status_register();
        if (!(status & JEDEC_RDSR_WIP)) {
            return true;
        }
    } while (timeout--);

    return false;
}

static void flash_write_enable(void) {
    qspi_set_write_protect(false);

    const uint8_t write_enable_cmd[] = {JEDEC_CMD_WREN};
    const qspi_xfer_desc_t write_enable_desc = {
        .mode = QSPI_MODE_SIO,
        .tx_data = write_enable_cmd,
        .tx_len = sizeof(write_enable_cmd),
        .dummy_cycles_after_tx = 0,
        .rx_data = NULL,
        .rx_len = 0
    };
    qspi_write_then_read(&write_enable_desc);
}

void flash_detect(void) {
    qspic_read_sfdp(&flash_info_g);
    debug_puts("Flash size ");
    debug_putlong(flash_info_g.size_bytes);
    debug_puts(" bytes\r\n");
}

uint32_t flash_get_size(void) {
    return flash_info_g.size_bytes;
}

void flash_read(const uint32_t address, void *dst, const uint32_t len) {
    uint8_t read_flash_cmd[] = {
        JEDEC_CMD_READ, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
    };
    const qspi_xfer_desc_t desc = {
        .mode = QSPI_MODE_SIO,
        .tx_data = read_flash_cmd,
        .tx_len = sizeof(read_flash_cmd),
        .dummy_cycles_after_tx = 0,
        .rx_data = dst,
        .rx_len = len
    };
    qspi_write_then_read(&desc);
}

bool flash_erase_sector(const uint32_t address) {
    flash_write_enable();

    uint8_t erase_sector_cmd[] = {
        JEDEC_CMD_SECTOR_ERASE, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
    };
    const qspi_xfer_desc_t erase_sector_desc = {
        .mode = QSPI_MODE_SIO,
        .tx_data = erase_sector_cmd,
        .tx_len = sizeof(erase_sector_cmd),
        .dummy_cycles_after_tx = 0,
        .rx_data = NULL,
        .rx_len = 0
    };
    qspi_write_then_read(&erase_sector_desc);

    const bool success = wait_flash_write_finished(TIMEOUT_SECTOR_ERASE);

    qspi_set_write_protect(true);

    return success;
}

bool flash_program_page(const uint32_t address, const void *page_data) {
    flash_write_enable();

    uint8_t program_page_cmd[] = {
        JEDEC_CMD_PAGE_PROGRAM, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
    };
    const qspi_xfer_action_t actions[] = {
        {
            .action = QSPI_WRITE,
            .tx_data = program_page_cmd,
            .len = sizeof(program_page_cmd)
        },
        {
            .action = QSPI_WRITE,
            .tx_data = page_data,
            .len = FLASH_PAGE_SIZE
        }
    };

    qspi_scatter_transfer(QSPI_MODE_SIO, actions, ARRAY_SIZE(actions));

    const bool success = wait_flash_write_finished(TIMEOUT_PAGE_PROGRAM);

    qspi_set_write_protect(true);

    return success;
}
