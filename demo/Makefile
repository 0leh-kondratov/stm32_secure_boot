# Demo with FreeRTOS: LEDs + UART at 0x08000000.
# FreeRTOS in project root ./FreeRTOS. Run from root: make demo
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS  = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS += -I common -I demo -I FreeRTOS/Source/include -I FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS += -DUART_LOG_USE_USART3
LDFLAGS = -T demo/demo.ld -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc

FREERTOS_SRC = FreeRTOS/Source
PORT         = $(FREERTOS_SRC)/portable/GCC/ARM_CM4F
MEMMANG      = $(FREERTOS_SRC)/portable/MemMang

# Ensure FreeRTOS uses demo's FreeRTOSConfig.h (FreeRTOS.h includes it from include/).
FREERTOS_INC = $(FREERTOS_SRC)/include
DEMO_CFG     = demo/FreeRTOSConfig.h

config-copied: $(DEMO_CFG)
	@cp "$(FREERTOS_INC)/FreeRTOSConfig.h" "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" 2>/dev/null || true
	@cp "$(DEMO_CFG)" "$(FREERTOS_INC)/FreeRTOSConfig.h"
	@touch config-copied

BUILD = build/demo
OBJ = $(BUILD)/main_freertos.o $(BUILD)/uart_log.o $(BUILD)/startup.o $(BUILD)/memset.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/timers.o \
      $(BUILD)/heap_4.o $(BUILD)/port.o

$(BUILD)/tasks.o: config-copied
$(BUILD)/queue.o: config-copied
$(BUILD)/list.o: config-copied
$(BUILD)/timers.o: config-copied
$(BUILD)/heap_4.o: config-copied
$(BUILD)/port.o: config-copied

all: $(BUILD)/demo.elf $(BUILD)/demo.bin
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main_freertos.o: demo/main_freertos.c demo/FreeRTOSConfig.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/uart_log.o: common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/memset.o: demo/memset.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/startup.o: app/startup_stm32h743xx.s | $(BUILD)
	$(CC) -mcpu=cortex-m7 -mthumb -c -o $@ $<

$(BUILD)/tasks.o: $(FREERTOS_SRC)/tasks.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/queue.o: $(FREERTOS_SRC)/queue.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/list.o: $(FREERTOS_SRC)/list.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/timers.o: $(FREERTOS_SRC)/timers.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/heap_4.o: $(MEMMANG)/heap_4.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/port.o: $(PORT)/port.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/demo.elf: $(OBJ) demo/demo.ld
	$(LD) $(CFLAGS) -o $@ $(filter-out demo/demo.ld,$^) $(LDFLAGS)

$(BUILD)/demo.bin: $(BUILD)/demo.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/demo.bin (flash at 0x08000000)"

clean:
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true
	@rm -f config-copied
	rm -rf $(BUILD)

.PHONY: all clean
