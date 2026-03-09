# Step 2: FreeRTOS + single DisplayTask (LCD counter every 1s). I2C1 PB8/PB9.
# make step2 → build/step2/step2.bin

TOP      := $(dir $(lastword $(MAKEFILE_LIST)))..
CUBE_ROOT ?= $(abspath $(TOP)/../STM32CubeH7)

CC       = arm-none-eabi-gcc
LD       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
AS       = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS   = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS  += -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY -DUART_LOG_USE_USART3
CFLAGS  += -I$(TOP)/common -I$(TOP)/step2
CFLAGS  += -I$(TOP)/FreeRTOS/Source/include -I$(TOP)/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Include -I$(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS  += -I$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/Inc

LDFLAGS  = -T$(TOP)/demo/demo.ld -nostdlib -Wl,--gc-sections -lgcc

BUILD    = build/step2
HAL_SRC  = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
CMSIS_DEVICE = $(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx
SYSTEM   = $(CMSIS_DEVICE)/Source/Templates/system_stm32h7xx.c
FREERTOS = $(TOP)/FreeRTOS/Source
PORT     = $(FREERTOS)/portable/GCC/ARM_CM4F
MEMMANG  = $(FREERTOS)/portable/MemMang
STARTUP  = $(TOP)/app/startup_stm32h743xx.s
STEP2_CFG = $(TOP)/step2/FreeRTOSConfig.h
FREERTOS_INC = $(TOP)/FreeRTOS/Source/include

OBJ = $(BUILD)/main.o $(BUILD)/uart_log.o $(BUILD)/lcd_1602_i2c.o $(BUILD)/memset.o $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o \
      $(BUILD)/hal.o $(BUILD)/hal_cortex.o $(BUILD)/hal_gpio.o $(BUILD)/hal_rcc.o $(BUILD)/hal_pwr.o $(BUILD)/hal_i2c.o $(BUILD)/hal_i2c_ex.o $(BUILD)/hal_dma.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/port.o $(BUILD)/heap_4.o

config-copied: $(STEP2_CFG)
	@cp "$(FREERTOS_INC)/FreeRTOSConfig.h" "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" 2>/dev/null || true
	@cp "$(STEP2_CFG)" "$(FREERTOS_INC)/FreeRTOSConfig.h"
	@touch config-copied

$(BUILD)/queue.o: config-copied
$(BUILD)/list.o: config-copied
$(BUILD)/heap_4.o: config-copied
$(BUILD)/port.o: config-copied

all: $(BUILD)/step2.elf $(BUILD)/step2.bin
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main.o: $(TOP)/step2/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/uart_log.o: $(TOP)/common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lcd_1602_i2c.o: $(TOP)/common/lcd_1602_i2c.c | $(BUILD)
	$(CC) $(CFLAGS) -DUSE_HAL_DRIVER -c -o $@ $<
$(BUILD)/memset.o: $(TOP)/demo/memset.c | $(BUILD)
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
$(BUILD)/hal_i2c.o: $(HAL_SRC)/stm32h7xx_hal_i2c.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_i2c_ex.o: $(HAL_SRC)/stm32h7xx_hal_i2c_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_dma.o: $(HAL_SRC)/stm32h7xx_hal_dma.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tasks.o: $(FREERTOS)/tasks.c config-copied | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/queue.o: $(FREERTOS)/queue.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/list.o: $(FREERTOS)/list.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/port.o: $(PORT)/port.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/heap_4.o: $(MEMMANG)/heap_4.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/step2.elf: $(OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/step2.bin: $(BUILD)/step2.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/step2.bin"

clean:
	@mv "$(FREERTOS_INC)/FreeRTOSConfig.h.bak" "$(FREERTOS_INC)/FreeRTOSConfig.h" 2>/dev/null || true
	@rm -f config-copied
	rm -rf $(BUILD)

.PHONY: all clean
