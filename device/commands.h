#pragma once

#include <stdint.h>

typedef struct cmd_handler cmd_handler_t;

const cmd_handler_t *command_lookup(uint8_t cmd);

unsigned int command_min_param_len(const cmd_handler_t *handler);

void command_dispatch(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len);
