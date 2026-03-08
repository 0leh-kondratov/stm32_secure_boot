# Secure Bootloader Makefile for STM32H743ZI (arm-none-eabi-gcc)
# Default build: ECDSA stub (no mbedTLS). Use for first test.
# For real signature verification, add mbedTLS and build with MBEDTLS=1 (see BUILD_RU.md).

CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-as
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

CFLAGS  = -mcpu=cortex-m7 -mthumb -std=c11 -Wall -Wextra -O2 -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -DUSE_ECDSA_STUB
CFLAGS += -I bootloader/inc -I common
LDFLAGS = -T bootloader/bootloader.ld -nostdlib -Wl,--gc-sections -Wl,-Map=bootloader/bootloader.map

BOOTLOADER_SRC = bootloader/src/main.c
BOOTLOADER_ASM = bootloader/startup_stm32h743xx.s
BOOTLOADER_OBJ = build/bootloader_main.o build/startup_stm32h743xx.o

all: build/bootloader.elf build/bootloader.bin

build:
	@mkdir -p build

build/bootloader_main.o: bootloader/src/main.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/startup_stm32h743xx.o: bootloader/startup_stm32h743xx.s | build
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

build/bootloader.elf: $(BOOTLOADER_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

build/bootloader.bin: build/bootloader.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf build

.PHONY: all clean
