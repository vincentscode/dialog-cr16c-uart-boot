#include "traps.h"

#include <stdint.h>

#include "debug.h"
#include "system.h"

volatile uint32_t trap_pc_save;

static void print_trap(const char *trap) {
    debug_puts("\r\n=======TRAP=========\r\n");
    debug_puts(trap);
    debug_puts(" at PC=0x");
    debug_putlong_hex(trap_pc_save);
    debug_puts("\r\n======ENDTRAP=======\r\n");
}

static void print_trap_reset(const char *trap) {
    print_trap(trap);
    system_reset();
}

void svc_trap(void) {
    print_trap("Supervisor call");
}

void dvz_trap(void) {
    print_trap_reset("Divide by zero");
}

void flg_trap(void) {
    print_trap("Flag");
}

void bpt_trap(void) {
    print_trap("Breakpoint");
    while (1) {}
}

void trc_trap(void) {
    print_trap("Trace");
}

void und_trap(void) {
    print_trap_reset("Undefined instruction");
    while (1) {}
}

void iad_trap(void) {
    print_trap_reset("Illegal address");
}

void dbg_trap(void) {
    print_trap("Debug");
}
