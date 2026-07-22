SRCS         := crt0.s vectors.s startup.c loader.c uart.c protocol.c debug.c traps.c commands.c flash.c crc32.c qspi.c system.c dma.c chipid.c clock.c display.c spi.c

CHIP         ?= SC14441
CHIPDEF      := -DCHIP_$(CHIP)

ENABLE_DISPLAY ?= 1
FEATUREDEF     := -DENABLE_DISPLAY=$(ENABLE_DISPLAY)

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
    $(CHIPDEF) \
    $(FEATUREDEF) \
    -I $(TARGET_INC) \
    -I $(GCC_LIB)/include

WARNFLAGS := \
    -Wall -Wextra \
    -Wimplicit-function-declaration \
    -Wredundant-decls \
    -Wmissing-prototypes \
    -Wstrict-prototypes \
    -Wundef -Wshadow \
    -Wno-unused-parameter -Wno-unused-function \
    -Werror=return-type

LDFLAGS := \
    -nostartfiles \
    -T loader.lds \
    -L $(TARGET_LIB) \
    -Wl,-lgcc \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,-L$(GCC_LIB)

.PHONY: all clean

all: loader.bin

loader.bin: loader
	$(OBJCOPY) -O binary $< $@

loader: $(SRCS) loader.lds
	$(CC) $(CFLAGS) $(WARNFLAGS) $(LDFLAGS) $(SRCS) -o $@

loader.lds: uart.lds.S chip_config.h
	$(CC) -E -P -x c $(CHIPDEF) -I . uart.lds.S -o $@

clean:
	rm -f loader loader.bin loader.lds
