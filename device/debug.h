#pragma once

void debug_puts(const char *ptr);

void debug_putint(unsigned int val);

void debug_putlong(unsigned long val);

void debug_putbyte_hex(unsigned char byt);

void debug_putint_hex(unsigned int i);

void debug_putlong_hex(unsigned long i);

void debug_hexdump(const void *ptr, unsigned int len);
