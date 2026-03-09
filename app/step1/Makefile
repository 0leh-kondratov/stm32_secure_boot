# Step 1: Hardware Baseline — I2C1 (PB8/PB9) + LCD 1602. No FreeRTOS.
# Run from project root: make step1   → build/step1/step1.bin
TOP      ?= $(CURDIR)
CUBE_ROOT ?= $(abspath $(TOP)/../STM32CubeH7)

CC       = arm-none-eabi-gcc
LD       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
AS       = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS   = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2
CFLAGS  += -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY -DUART_LOG_USE_USART3
CFLAGS  += -I$(TOP)/common
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc
CFLAGS  += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS  += -I$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/Inc

LDFLAGS  = -T$(TOP)/app/demo/demo.ld -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc

BUILD    = build/step1
HAL_SRC  = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
STARTUP  = $(TOP)/app/startup_stm32h743xx.s
CMSIS_DEVICE = $(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx
SYSTEM   = $(CMSIS_DEVICE)/Source/Templates/system_stm32h7xx.c

OBJ = $(BUILD)/main.o $(BUILD)/uart_log.o $(BUILD)/lcd_1602_i2c.o $(BUILD)/startup.o $(BUILD)/system_stm32h7xx.o \
      $(BUILD)/hal_cortex.o $(BUILD)/hal_gpio.o $(BUILD)/hal_rcc.o $(BUILD)/hal_i2c.o $(BUILD)/hal_i2c_ex.o $(BUILD)/hal_dma.o \
      $(BUILD)/hal.o $(BUILD)/hal_pwr.o

all: $(BUILD)/step1.elf $(BUILD)/step1.bin

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/main.o: $(TOP)/app/step1/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/uart_log.o: $(TOP)/common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lcd_1602_i2c.o: $(TOP)/common/lcd_1602_i2c.c | $(BUILD)
	$(CC) $(CFLAGS) -DUSE_HAL_DRIVER -c -o $@ $<
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

$(BUILD)/step1.elf: $(OBJ)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD)/step1.bin: $(BUILD)/step1.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/step1.bin — flash at 0x08000000"

clean:
	rm -rf $(BUILD)

.PHONY: all clean
