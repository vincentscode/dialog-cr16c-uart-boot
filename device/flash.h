#pragma once

#include <stdbool.h>
#include <stdint.h>

void flash_detect(void);

uint32_t flash_get_size(void);

void flash_read(uint32_t address, void *dst, uint32_t len);

bool flash_erase_sector(uint32_t address);

bool flash_program_page(uint32_t address, const void *page_data);
