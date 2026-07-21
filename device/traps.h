#pragma once

#include <stdint.h>

/* populated by the vectors in vectors.s */
extern volatile uint32_t trap_pc_save;

void svc_trap(void);

void dvz_trap(void);

void flg_trap(void);

void bpt_trap(void);

void trc_trap(void);

void und_trap(void);

void iad_trap(void);

void dbg_trap(void);
