# Application Makefile for STM32H743ZI. Run from project root: make -f app/Makefile all
CC   = arm-none-eabi-gcc
LD   = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m7 -mthumb -std=c11 -Wall -O2 -I common
LDFLAGS = -T app/app.ld -nostdlib -Wl,--gc-sections

# USART3 (PD8/PD9) = ST-Link VCP, same as bootloader
APP_OBJ = build_app/app_main.o build_app/uart_log_usart3.o build_app/startup_app.o

all: build_app/app.elf build_app/app.bin

build_app:
	@mkdir -p build_app

build_app/app_main.o: app/main.c | build_app
	$(CC) $(CFLAGS) -c -o $@ $<

build_app/startup_app.o: app/startup_stm32h743xx.s | build_app
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

build_app/uart_log.o: common/uart_log.c | build_app
	$(CC) $(CFLAGS) -c -o $@ $<

build_app/uart_log_usart3.o: common/uart_log.c | build_app
	$(CC) $(CFLAGS) -DUART_LOG_USE_USART3 -c -o $@ $<

build_app/app.elf: $(APP_OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build_app/app.bin: build_app/app.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf build_app

.PHONY: all clean
