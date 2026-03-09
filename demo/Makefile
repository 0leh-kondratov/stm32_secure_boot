# Standalone demo: LEDs + UART log at 0x08000000. Run from project root: make -f demo/Makefile
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS  = -mcpu=cortex-m7 -mthumb -std=c11 -Wall -O2 -I common
CFLAGS += -DUART_LOG_USE_USART3
LDFLAGS = -T demo/demo.ld -nostdlib -Wl,--gc-sections

OBJ = build_demo/main.o build_demo/uart_log.o build_demo/startup.o

all: build_demo/demo.elf build_demo/demo.bin

build_demo:
	@mkdir -p build_demo

build_demo/main.o: demo/main.c | build_demo
	$(CC) $(CFLAGS) -c -o $@ $<

build_demo/uart_log.o: common/uart_log.c | build_demo
	$(CC) $(CFLAGS) -c -o $@ $<

build_demo/startup.o: app/startup_stm32h743xx.s | build_demo
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

build_demo/demo.elf: $(OBJ) demo/demo.ld
	$(LD) $(CFLAGS) -o $@ $(filter-out demo/demo.ld,$^) $(LDFLAGS)

build_demo/demo.bin: build_demo/demo.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: build_demo/demo.bin (flash at 0x08000000)"

clean:
	rm -rf build_demo

.PHONY: all clean
