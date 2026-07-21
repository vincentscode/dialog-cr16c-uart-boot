SRCS         := crt0.s vectors.s startup.c loader.c uart.c protocol.c debug.c traps.c commands.c flash.c crc32.c qspi.c system.c dma.c chipid.c display.c spi.c clock.c

TOOLCHAIN    ?= $(HOME)/opt/cross
GCC_VERSION  ?= 10.5.0

CC           := $(TOOLCHAIN)/bin/cr16-c-elf-gcc
OBJCOPY      := $(TOOLCHAIN)/bin/cr16-c-elf-objcopy

TARGET_LIB   := $(TOOLCHAIN)/cr16-c-elf/lib
TARGET_INC   := $(TOOLCHAIN)/cr16-c-elf/include
GCC_LIB      := $(TOOLCHAIN)/lib/gcc/cr16-c-elf/$(GCC_VERSION)

CFLAGS := \
    -mcr16c \
    -Os -flto -ggdb \
    -I $(TARGET_INC) \
    -I $(GCC_LIB)/include

WARNFLAGS := \
    -Wall -Wextra \
    -Wimplicit-function-declaration \
    -Wredundant-decls \
    -Wmissing-prototypes \
    -Wstrict-prototypes \
    -Wundef -Wshadow \
    -Wno-unused \
    -Werror=return-type

LDFLAGS := \
    -nostartfiles \
    -T sc14441-uart.ld \
    -L $(TARGET_LIB) \
    -Wl,-lgcc \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,-L$(GCC_LIB)

.PHONY: all clean

all: loader.bin

loader.bin: loader
	$(OBJCOPY) -O binary $< $@

loader: $(SRCS)
	$(CC) $(CFLAGS) $(WARNFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f loader loader.bin
