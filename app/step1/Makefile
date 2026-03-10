# Step1: FreeRTOS — Logger (xLogQueue + HAL_UART_Transmit), LED blink, App task. NUCLEO-H743ZI.
# For board: use official Template linker + startup + system_stm32h7xx.c (same as demo-official).
# For Renode: use demo.ld + our startup + CMSIS system.
TOP      ?= $(CURDIR)
CUBE_ROOT ?= $(abspath $(TOP)/../STM32CubeH7)
TEMPLATE_PRJ = $(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates

CC       = arm-none-eabi-gcc
LD       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
AS       = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS   = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS  += -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY
CFLAGS  += -I$(TOP)/app/step1/Inc
CFLAGS  += -I$(TOP)/FreeRTOS/Source/include
CFLAGS  += -I$(TOP)/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS  += -I$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/Inc
ifneq ($(STEP1_RENODE),1)
# Board: official Template files
LDSCRIPT = $(TOP)/app/step1/STM32H743ZITx_FLASH.ld
STARTUP  = $(TEMPLATE_PRJ)/STM32CubeIDE/Example/Startup/startup_stm32h743zitx.s
SYSTEM   = $(TEMPLATE_PRJ)/Src/system_stm32h7xx.c
LDFLAGS  = -T$(LDSCRIPT) -Wl,--gc-sections -specs=nano.specs -lc -lm -lnosys
else
# Renode: our minimal startup and demo.ld
LDSCRIPT = $(TOP)/app/demo/demo.ld
STARTUP  = $(TOP)/app/startup_stm32h743xx.s
CMSIS_DEVICE = $(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx
SYSTEM   = $(CMSIS_DEVICE)/Source/Templates/system_stm32h7xx.c
LDFLAGS  = -T$(LDSCRIPT) -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc
CFLAGS  += -DSTEP1_RENODE
endif

BUILD    = build/step1
HAL_SRC  = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
FREERTOS_SRC = $(TOP)/FreeRTOS/Source
PORT     = $(FREERTOS_SRC)/portable/GCC/ARM_CM4F
MEMMANG  = $(FREERTOS_SRC)/portable/MemMang
FREERTOS_INC = $(FREERTOS_SRC)/include
STEP1_CFG    = $(TOP)/app/step1/FreeRTOSConfig.h

# Always copy step1 config when building step1 (so step2's config doesn't break step1)
force-step1-config:
	@cp "$(FREERTOS_INC)/FreeRTOSConfig.h" "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" 2>/dev/null || true
	@cp "$(STEP1_CFG)" "$(FREERTOS_INC)/FreeRTOSConfig.h"
	@touch config-copied

config-copied: force-step1-config

OBJ = $(BUILD)/main.o $(BUILD)/stm32h7xx_hal_msp.o $(BUILD)/stm32h7xx_it.o $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o \
      $(BUILD)/hal_cortex.o $(BUILD)/hal_gpio.o $(BUILD)/hal_rcc.o $(BUILD)/hal_rcc_ex.o \
      $(BUILD)/hal_uart.o $(BUILD)/hal_uart_ex.o $(BUILD)/hal_dma.o $(BUILD)/hal.o $(BUILD)/hal_pwr.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/timers.o $(BUILD)/heap_4.o $(BUILD)/port.o

$(BUILD)/tasks.o: config-copied
$(BUILD)/queue.o: config-copied
$(BUILD)/list.o: config-copied
$(BUILD)/timers.o: config-copied
$(BUILD)/heap_4.o: config-copied
$(BUILD)/port.o: config-copied

all: force-step1-config config-copied $(BUILD)/step1.elf $(BUILD)/step1.bin
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main.o: $(TOP)/app/step1/main.c $(TOP)/app/step1/FreeRTOSConfig.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_hal_msp.o: $(TOP)/app/step1/stm32h7xx_hal_msp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_it.o: $(TOP)/app/step1/stm32h7xx_it.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	$(AS) $(CFLAGS) -c -o $@ $<
$(BUILD)/system_stm32h7xx.o: $(SYSTEM) | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/hal.o: $(HAL_SRC)/stm32h7xx_hal.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_cortex.o: $(HAL_SRC)/stm32h7xx_hal_cortex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_gpio.o: $(HAL_SRC)/stm32h7xx_hal_gpio.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_rcc.o: $(HAL_SRC)/stm32h7xx_hal_rcc.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_pwr.o: $(HAL_SRC)/stm32h7xx_hal_pwr.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_uart.o: $(HAL_SRC)/stm32h7xx_hal_uart.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_uart_ex.o: $(HAL_SRC)/stm32h7xx_hal_uart_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_rcc_ex.o: $(HAL_SRC)/stm32h7xx_hal_rcc_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_dma.o: $(HAL_SRC)/stm32h7xx_hal_dma.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

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

$(BUILD)/step1.elf: $(OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/step1.bin: $(BUILD)/step1.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/step1.bin — flash at 0x08000000"

clean:
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true
	@rm -f config-copied
	rm -rf $(BUILD)

# Сборка для Renode: HSI 64 MHz, без настройки HSE (early return в SystemClock_Config)
renode: clean-step1-main
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) all STEP1_RENODE=1 TOP="$(TOP)" CUBE_ROOT="$(CUBE_ROOT)"
clean-step1-main:
	@rm -f $(BUILD)/main.o $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o

.PHONY: all clean renode clean-step1-main
