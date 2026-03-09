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

BOOTLOADER_SRC = bootloader/src/main.c common/uart_log.c
BOOTLOADER_ASM = bootloader/startup_stm32h743xx.s
# Use USART3 (PD8/PD9) for debug log — ST-Link VCP on NUCLEO-H743ZI2
BOOTLOADER_OBJ = build/bootloader_main.o build/uart_log_usart3.o build/startup_stm32h743xx.o

all: build/bootloader.elf build/bootloader.bin

build:
	@mkdir -p build

build/bootloader_main.o: bootloader/src/main.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/startup_stm32h743xx.o: bootloader/startup_stm32h743xx.s | build
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

build/uart_log.o: common/uart_log.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/uart_log_usart3.o: common/uart_log.c | build
	$(CC) $(CFLAGS) -DUART_LOG_USE_USART3 -c -o $@ $<

build/bootloader.elf: $(BOOTLOADER_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

build/bootloader.bin: build/bootloader.elf
	$(OBJCOPY) -O binary $< $@

# Minimal bootloader: only UART log test (no signature, no jump to app)
MINIMAL_OBJ = build/bootloader_minimal_main.o build/uart_log.o build/startup_stm32h743xx.o

build/bootloader_minimal_main.o: bootloader/src/main_minimal.c | build
	$(CC) $(CFLAGS) -c -o $@ $<

build/bootloader_minimal.elf: $(MINIMAL_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

build/bootloader_minimal.bin: build/bootloader_minimal.elf
	$(OBJCOPY) -O binary $< $@

minimal: build/bootloader_minimal.bin

# Minimal bootloader with USART3 (PD8/PD9) for boards where VCP is on USART3
build/bootloader_minimal_usart3.elf: build/bootloader_minimal_main.o build/uart_log_usart3.o build/startup_stm32h743xx.o
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

build/bootloader_minimal_usart3.bin: build/bootloader_minimal_usart3.elf
	$(OBJCOPY) -O binary $< $@

minimal-usart3: build/bootloader_minimal_usart3.bin

clean:
	rm -rf build

.PHONY: all clean minimal minimal-usart3
