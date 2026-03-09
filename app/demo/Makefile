# Demo with FreeRTOS: LEDs + UART at 0x08000000.
# Run from root: make demo   (build/demo/demo.bin)
# TOP = project root (run make from repo root)
TOP ?= $(CURDIR)
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CUBE_ROOT ?= $(abspath $(TOP)/../STM32CubeH7)
CUBE_EXISTS = $(wildcard $(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include/stm32h7xx.h)

CFLAGS  = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS += -I $(TOP)/common -I $(TOP)/app/demo -I $(TOP)/FreeRTOS/Source/include -I $(TOP)/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS += -DUART_LOG_USE_USART3 -DUART_LOG_FREERTOS_YIELD
ifneq ($(CUBE_EXISTS),)
  CFLAGS += -I $(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include -I $(CUBE_ROOT)/Drivers/CMSIS/Include -DSTM32H743xx -DDEMO_USE_SYSTEM_CLOCK
endif
ifeq ($(DEMO_RENODE),1)
  CFLAGS += -DDEMO_RENODE_AUTO_CMD
endif
LDFLAGS = -T $(TOP)/app/demo/demo.ld -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc

FREERTOS_SRC = $(TOP)/FreeRTOS/Source
PORT         = $(FREERTOS_SRC)/portable/GCC/ARM_CM4F
MEMMANG      = $(FREERTOS_SRC)/portable/MemMang

FREERTOS_INC = $(FREERTOS_SRC)/include
DEMO_CFG     = $(TOP)/app/demo/FreeRTOSConfig.h

config-copied: $(DEMO_CFG)
	@cp "$(FREERTOS_INC)/FreeRTOSConfig.h" "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" 2>/dev/null || true
	@cp "$(DEMO_CFG)" "$(FREERTOS_INC)/FreeRTOSConfig.h"
	@touch config-copied

BUILD = build/demo
OBJ = $(BUILD)/main_freertos.o $(BUILD)/uart_log.o $(BUILD)/startup.o $(BUILD)/memset.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/timers.o \
      $(BUILD)/heap_4.o $(BUILD)/port.o
ifneq ($(CUBE_EXISTS),)
  ifneq ($(DEMO_RENODE),1)
    OBJ += $(BUILD)/system_stm32h7xx.o
  endif
endif

$(BUILD)/tasks.o: config-copied
$(BUILD)/queue.o: config-copied
$(BUILD)/list.o: config-copied
$(BUILD)/timers.o: config-copied
$(BUILD)/heap_4.o: config-copied
$(BUILD)/port.o: config-copied

all: config-copied $(BUILD)/demo.elf $(BUILD)/demo.bin
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main_freertos.o: $(TOP)/app/demo/main_freertos.c $(TOP)/app/demo/FreeRTOSConfig.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/uart_log.o: $(TOP)/common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/memset.o: $(TOP)/app/demo/memset.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/startup.o: $(TOP)/app/startup_stm32h743xx.s | $(BUILD)
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

ifneq ($(CUBE_EXISTS),)
  ifneq ($(DEMO_RENODE),1)
$(BUILD)/system_stm32h7xx.o: $(FREERTOS_SRC)/system_stm32h7xx.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
  endif
endif

$(BUILD)/demo.elf: $(OBJ) $(TOP)/app/demo/demo.ld
	$(LD) $(CFLAGS) -o $@ $(filter-out $(TOP)/app/demo/demo.ld,$^) $(LDFLAGS)

$(BUILD)/demo.bin: $(BUILD)/demo.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/demo.bin (flash at 0x08000000)"

clean:
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true
	@rm -f config-copied
	rm -rf $(BUILD)

# Сборка для Renode: раз в 5 с вывод status и переключение LED3 (обход бага UART RX в Renode, issue #487)
renode: clean-main
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) all DEMO_RENODE=1 TOP="$(TOP)" CUBE_ROOT="$(CUBE_ROOT)"
clean-main:
	@rm -f $(BUILD)/main_freertos.o $(BUILD)/uart_log.o

.PHONY: all clean renode clean-main
