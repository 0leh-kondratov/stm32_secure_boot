# Application Makefile for STM32H743ZI. Run from project root: make -f app/Makefile all
CC   = arm-none-eabi-gcc
LD   = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m7 -mthumb -std=c11 -Wall -O2 -I common
LDFLAGS = -T app/app.ld -nostdlib -Wl,--gc-sections

# USART3 (PD8/PD9) = ST-Link VCP, same as bootloader
BUILD   = build/app
APP_OBJ = $(BUILD)/app_main.o $(BUILD)/uart_log_usart3.o $(BUILD)/startup_app.o

all: $(BUILD)/app.elf $(BUILD)/app.bin

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/app_main.o: app/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/startup_app.o: app/startup_stm32h743xx.s | $(BUILD)
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

$(BUILD)/uart_log.o: common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/uart_log_usart3.o: common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -DUART_LOG_USE_USART3 -c -o $@ $<

$(BUILD)/app.elf: $(APP_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/app.bin: $(BUILD)/app.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean
