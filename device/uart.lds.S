#include "chip_config.h"

EXTERN(_vector_table)
ENTRY(bootloader_entry)
MEMORY
{
 RES (r)    : ORIGIN = CHIP_RAM_ORIGIN,                   LENGTH = CHIP_RES_LENGTH
 ram (rwx)  : ORIGIN = CHIP_RAM_ORIGIN + CHIP_RES_LENGTH, LENGTH = CHIP_RAM_LENGTH - CHIP_RES_LENGTH
}
SECTIONS
{
 .text : {
  *(.text.crt0)
  . = ALIGN(4);
  *(.text*)
  . = ALIGN(4);
  *(.rodata*)
  . = ALIGN(4);
  __intbase = .;
  KEEP (*(.vectors))
  . = ALIGN(4);
  __ctors_start = .;
  KEEP (*(.ctors))
  __ctors_end = .;
 } >ram
 . = ALIGN(4);
 __etext = .;
 .data : {
  __data = .;
  *(.data*)
  *(.ramtext*)
  . = ALIGN(4);
  __edata = .;
 } >ram
 __data_loadaddr = LOADADDR(.data);
 .bss (NOLOAD) : {
  . = ALIGN(4);
  __bss = .;
  *(.bss*)
  *(COMMON)
  . = ALIGN(4);
  __ebss = .;
 } >ram
 .noinit (NOLOAD) : {
  *(.noinit*)
 } >ram
 . = ALIGN(4);
 end = .;
 __istack = ORIGIN(ram) + LENGTH(ram);
 __ustack = __istack - 0x100;
}
