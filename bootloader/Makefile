# Secure Bootloader Makefile for STM32H743ZI (arm-none-eabi-gcc)
# Default build: ECDSA stub (no mbedTLS). Use for first test.
# For real signature verification, add mbedTLS and build with MBEDTLS=1 (see BUILD_PL.md).

CC      = arm-none-eabi-gcc
AS      = arm-none-eabi-as
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

CFLAGS  = -mcpu=cortex-m7 -mthumb -std=c11 -Wall -Wextra -O2 -ffunction-sections -fdata-sections -fno-builtin
CFLAGS += -DUSE_ECDSA_STUB
CFLAGS += -I bootloader/inc -I common
BUILD   = build/bootloader
LDFLAGS = -T bootloader/bootloader.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(BUILD)/bootloader.map

# Debug build for GDB: -g -O0 (make -f bootloader/Makefile debug)
# debug-clean: odbuduj od podstaw za pomocą symboli debugowania
debug: CFLAGS += -g -O0
debug: all

debug-clean: clean
	$(MAKE) debug

BOOTLOADER_SRC = bootloader/src/main.c common/uart_log.c
BOOTLOADER_ASM = bootloader/startup_stm32h743xx.s
# Use USART3 (PD8/PD9) for debug log — ST-Link VCP on NUCLEO-H743ZI2
BOOTLOADER_OBJ = $(BUILD)/bootloader_main.o $(BUILD)/uart_log_usart3.o $(BUILD)/startup_stm32h743xx.o

all: $(BUILD)/bootloader.elf $(BUILD)/bootloader.bin

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/bootloader_main.o: bootloader/src/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/startup_stm32h743xx.o: bootloader/startup_stm32h743xx.s | $(BUILD)
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

$(BUILD)/uart_log.o: common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/uart_log_usart3.o: common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -DUART_LOG_USE_USART3 -c -o $@ $<

$(BUILD)/bootloader.elf: $(BOOTLOADER_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

$(BUILD)/bootloader.bin: $(BUILD)/bootloader.elf
	$(OBJCOPY) -O binary $< $@

# Minimal bootloader: only UART log test (no signature, no jump to app)
MINIMAL_OBJ = $(BUILD)/bootloader_minimal_main.o $(BUILD)/uart_log.o $(BUILD)/startup_stm32h743xx.o

$(BUILD)/bootloader_minimal_main.o: bootloader/src/main_minimal.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/bootloader_minimal.elf: $(MINIMAL_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

$(BUILD)/bootloader_minimal.bin: $(BUILD)/bootloader_minimal.elf
	$(OBJCOPY) -O binary $< $@

minimal: $(BUILD)/bootloader_minimal.bin

# Minimal bootloader with USART3 (PD8/PD9) for boards where VCP is on USART3
$(BUILD)/bootloader_minimal_usart3.elf: $(BUILD)/bootloader_minimal_main.o $(BUILD)/uart_log_usart3.o $(BUILD)/startup_stm32h743xx.o
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

$(BUILD)/bootloader_minimal_usart3.bin: $(BUILD)/bootloader_minimal_usart3.elf
	$(OBJCOPY) -O binary $< $@

minimal-usart3: $(BUILD)/bootloader_minimal_usart3.bin

clean:
	rm -rf $(BUILD)

.PHONY: all clean minimal minimal-usart3 debug debug-clean
