# Step2: FreeRTOS + Logger + LED + I2C scanner. NUCLEO-H743ZI.
# Same layout as step1, adds I2C1 (PB6/PB7) and scan result to UART log.
TOP      ?= $(CURDIR)
CUBE_ROOT ?= $(abspath $(TOP)/../STM32CubeH7)
TEMPLATE_PRJ = $(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates

CC       = arm-none-eabi-gcc
LD       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
AS       = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS   = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS  += -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY
CFLAGS  += -I$(TOP)/app/step2/Inc
CFLAGS  += -I$(TOP)/FreeRTOS/Source/include
CFLAGS  += -I$(TOP)/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS  += -I$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/Inc
ifneq ($(STEP2_RENODE),1)
# Use step1 linker and startup so layout is identical to step1 (same ROM/RAM)
LDSCRIPT = $(TOP)/app/step1/STM32H743ZITx_FLASH.ld
STARTUP  = $(TEMPLATE_PRJ)/STM32CubeIDE/Example/Startup/startup_stm32h743zitx.s
SYSTEM   = $(TEMPLATE_PRJ)/Src/system_stm32h7xx.c
LDFLAGS  = -T$(LDSCRIPT) -Wl,--gc-sections -specs=nano.specs -lc -lm -lnosys
else
LDSCRIPT = $(TOP)/app/demo/demo.ld
STARTUP  = $(TOP)/app/startup_stm32h743xx.s
CMSIS_DEVICE = $(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx
SYSTEM   = $(CMSIS_DEVICE)/Source/Templates/system_stm32h7xx.c
LDFLAGS  = -T$(LDSCRIPT) -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc
CFLAGS  += -DSTEP2_RENODE
endif

BUILD    = build/step2
HAL_SRC  = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
FREERTOS_SRC = $(TOP)/FreeRTOS/Source
PORT     = $(FREERTOS_SRC)/portable/GCC/ARM_CM4F
MEMMANG  = $(FREERTOS_SRC)/portable/MemMang
FREERTOS_INC = $(FREERTOS_SRC)/include
STEP2_CFG    = $(TOP)/app/step2/FreeRTOSConfig.h

config-copied: $(STEP2_CFG)
	@cp "$(FREERTOS_INC)/FreeRTOSConfig.h" "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" 2>/dev/null || true
	@cp "$(STEP2_CFG)" "$(FREERTOS_INC)/FreeRTOSConfig.h"
	@touch config-copied

OBJ = $(BUILD)/main.o $(BUILD)/stm32h7xx_hal_msp.o $(BUILD)/stm32h7xx_it.o $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o \
      $(BUILD)/hal_cortex.o $(BUILD)/hal_gpio.o $(BUILD)/hal_rcc.o $(BUILD)/hal_rcc_ex.o \
      $(BUILD)/hal_uart.o $(BUILD)/hal_uart_ex.o $(BUILD)/hal_i2c.o $(BUILD)/hal_i2c_ex.o $(BUILD)/hal_dma.o \
      $(BUILD)/hal.o $(BUILD)/hal_pwr.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/timers.o $(BUILD)/heap_4.o $(BUILD)/port.o

$(BUILD)/tasks.o: config-copied
$(BUILD)/queue.o: config-copied
$(BUILD)/list.o: config-copied
$(BUILD)/timers.o: config-copied
$(BUILD)/heap_4.o: config-copied
$(BUILD)/port.o: config-copied

all: config-copied $(BUILD)/step2.elf $(BUILD)/step2.bin
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main.o: $(TOP)/app/step2/main.c $(TOP)/app/step2/FreeRTOSConfig.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_hal_msp.o: $(TOP)/app/step2/stm32h7xx_hal_msp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_it.o: $(TOP)/app/step2/stm32h7xx_it.c | $(BUILD)
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
$(BUILD)/hal_i2c.o: $(HAL_SRC)/stm32h7xx_hal_i2c.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_i2c_ex.o: $(HAL_SRC)/stm32h7xx_hal_i2c_ex.c | $(BUILD)
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

$(BUILD)/step2.elf: $(OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/step2.bin: $(BUILD)/step2.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/step2.bin — flash at 0x08000000"

clean:
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true
	@rm -f config-copied
	rm -rf $(BUILD)

renode: clean-step2-main
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) all STEP2_RENODE=1 TOP="$(TOP)" CUBE_ROOT="$(CUBE_ROOT)"
clean-step2-main:
	@rm -f $(BUILD)/main.o $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o

.PHONY: all clean renode clean-step2-main
