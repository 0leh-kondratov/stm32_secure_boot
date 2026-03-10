# LwIP + FreeRTOS build for NUCLEO-H743ZI (STM32H743ZI2).
# Requires CUBE_ROOT pointing to STM32CubeH7 with Drivers and Middlewares.
# If Middlewares/Third_Party/LwIP or FreeRTOS are empty, copy from full Cube package.
# Run from project root: make lwip

TOP     ?= $(CURDIR)
CUBE_ROOT ?= $(abspath $(TOP)/../STM32CubeH7)

CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
AS      = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS  = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2 -g
CFLAGS += -DDEBUG -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_MB1364
CFLAGS += -DLWIP_BUILD -DUART_LOG_USE_USART3
CFLAGS += -I$(TOP)/common
CFLAGS += -I$(TOP)/app/lwip/Inc
CFLAGS += -I$(TOP)/FreeRTOS/Source/include
CFLAGS += -I$(TOP)/FreeRTOS/Source/portable/GCC/ARM_CM4F
CFLAGS += -I$(CUBE_ROOT)/Drivers/CMSIS/Include
CFLAGS += -I$(CUBE_ROOT)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS += -I$(CUBE_ROOT)/Drivers/CMSIS/RTOS2/Include
CFLAGS += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc
CFLAGS += -I$(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS += -I$(CUBE_ROOT)/Drivers/BSP/STM32H7xx_Nucleo
CFLAGS += -I$(CUBE_ROOT)/Drivers/BSP/Components/Common
CFLAGS += -I$(CUBE_ROOT)/Drivers/BSP/Components/lan8742
CFLAGS += -I$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/src/include
CFLAGS += -I$(CUBE_ROOT)/Middlewares/Third_Party/LwIP/system
CFLAGS += -I$(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source/include
CFLAGS += -I$(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2

LDFLAGS = -T$(TOP)/app/lwip/lwip_nucleo_h743zi.ld -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc -lnosys

BUILD   = build/lwip
FREERTOS_SRC = $(TOP)/FreeRTOS/Source
PORT    = $(FREERTOS_SRC)/portable/GCC/ARM_CM4F
MEMMANG = $(FREERTOS_SRC)/portable/MemMang

# HAL drivers (minimal set for ETH + UART + timebase)
HAL_SRC = $(CUBE_ROOT)/Drivers/STM32H7xx_HAL_Driver/Src
# BSP
BSP_SRC = $(CUBE_ROOT)/Drivers/BSP/STM32H7xx_Nucleo
LAN8742_SRC = $(CUBE_ROOT)/Drivers/BSP/Components/lan8742
# LwIP
LWIP_SRC = $(CUBE_ROOT)/Middlewares/Third_Party/LwIP/src
LWIP_API = $(LWIP_SRC)/api
LWIP_CORE = $(LWIP_SRC)/core
LWIP_NETIF = $(LWIP_SRC)/netif
LWIP_SYS = $(CUBE_ROOT)/Middlewares/Third_Party/LwIP/system
LWIP_ARCH = $(LWIP_SYS)/arch
LWIP_IPV4 = $(LWIP_SRC)/core/ipv4
# FreeRTOS Cube (CMSIS-RTOS2)
FREERTOS_CUBE = $(CUBE_ROOT)/Middlewares/Third_Party/FreeRTOS/Source
STARTUP = $(TOP)/app/startup_stm32h743xx.s

# Object list: startup, app, FreeRTOS kernel, port, heap, cmsis_os2, HAL, BSP, LwIP
OBJ = $(BUILD)/startup.o \
      $(BUILD)/main.o $(BUILD)/ethernetif.o $(BUILD)/app_ethernet.o $(BUILD)/tcp_echo_server.o \
      $(BUILD)/stm32h7xx_it.o $(BUILD)/system_stm32h7xx.o $(BUILD)/stm32h7xx_hal_timebase_tim.o $(BUILD)/uart_log.o \
      $(BUILD)/tasks.o $(BUILD)/queue.o $(BUILD)/list.o $(BUILD)/timers.o $(BUILD)/event_groups.o $(BUILD)/stream_buffer.o \
      $(BUILD)/port.o $(BUILD)/heap_4.o $(BUILD)/cmsis_os2.o \
      $(BUILD)/hal.o $(BUILD)/hal_cortex.o $(BUILD)/hal_dma.o $(BUILD)/hal_eth.o $(BUILD)/hal_exti.o $(BUILD)/hal_flash.o \
      $(BUILD)/hal_gpio.o $(BUILD)/hal_pwr.o $(BUILD)/hal_rcc.o $(BUILD)/hal_tim.o $(BUILD)/hal_uart.o \
      $(BUILD)/hal_rcc_ex.o $(BUILD)/hal_pwr_ex.o $(BUILD)/hal_uart_ex.o $(BUILD)/hal_tim_ex.o $(BUILD)/hal_flash_ex.o \
      $(BUILD)/bsp_nucleo.o $(BUILD)/lan8742.o \
      $(BUILD)/lwip_api_lib.o $(BUILD)/lwip_api_msg.o $(BUILD)/lwip_err.o $(BUILD)/lwip_netbuf.o $(BUILD)/lwip_netdb.o \
      $(BUILD)/lwip_netifapi.o $(BUILD)/lwip_sockets.o $(BUILD)/lwip_tcpip.o \
      $(BUILD)/lwip_def.o $(BUILD)/lwip_dns.o $(BUILD)/lwip_inet_chksum.o $(BUILD)/lwip_init.o $(BUILD)/lwip_ip.o \
      $(BUILD)/lwip_mem.o $(BUILD)/lwip_memp.o $(BUILD)/lwip_pbuf.o $(BUILD)/lwip_raw.o $(BUILD)/lwip_stats.o $(BUILD)/lwip_sys.o \
      $(BUILD)/lwip_altcp.o $(BUILD)/lwip_altcp_alloc.o $(BUILD)/lwip_altcp_tcp.o \
      $(BUILD)/lwip_tcp.o $(BUILD)/lwip_tcp_in.o $(BUILD)/lwip_tcp_out.o $(BUILD)/lwip_udp.o $(BUILD)/lwip_netif.o \
      $(BUILD)/lwip_ip4.o $(BUILD)/lwip_etharp.o $(BUILD)/lwip_icmp.o $(BUILD)/lwip_dhcp.o $(BUILD)/lwip_acd.o $(BUILD)/lwip_timeouts.o \
      $(BUILD)/lwip_ip4_addr.o $(BUILD)/lwip_ip4_frag.o \
      $(BUILD)/lwip_ethernet.o $(BUILD)/lwip_slip.o $(BUILD)/lwip_sys_arch.o

all: $(BUILD)/lwip.elf $(BUILD)/lwip.bin

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	$(AS) $(CFLAGS) -c -o $@ $<

$(BUILD)/main.o: $(TOP)/app/lwip/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/ethernetif.o: $(FREERTOS_SRC)/ethernetif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/app_ethernet.o: $(FREERTOS_SRC)/app_ethernet.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/tcp_echo_server.o: $(FREERTOS_SRC)/tcp_echo_server.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_it.o: $(FREERTOS_SRC)/stm32h7xx_it.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/system_stm32h7xx.o: $(FREERTOS_SRC)/system_stm32h7xx.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_hal_timebase_tim.o: $(FREERTOS_SRC)/stm32h7xx_hal_timebase_tim.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/uart_log.o: $(TOP)/common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tasks.o: $(FREERTOS_CUBE)/tasks.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/queue.o: $(FREERTOS_CUBE)/queue.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/list.o: $(FREERTOS_CUBE)/list.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/timers.o: $(FREERTOS_CUBE)/timers.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/event_groups.o: $(FREERTOS_CUBE)/event_groups.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stream_buffer.o: $(FREERTOS_CUBE)/stream_buffer.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/port.o: $(PORT)/port.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/heap_4.o: $(MEMMANG)/heap_4.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/cmsis_os2.o: $(FREERTOS_CUBE)/CMSIS_RTOS_V2/cmsis_os2.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/hal.o: $(HAL_SRC)/stm32h7xx_hal.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_cortex.o: $(HAL_SRC)/stm32h7xx_hal_cortex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_dma.o: $(HAL_SRC)/stm32h7xx_hal_dma.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_eth.o: $(HAL_SRC)/stm32h7xx_hal_eth.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_exti.o: $(HAL_SRC)/stm32h7xx_hal_exti.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_flash.o: $(HAL_SRC)/stm32h7xx_hal_flash.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_gpio.o: $(HAL_SRC)/stm32h7xx_hal_gpio.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_pwr.o: $(HAL_SRC)/stm32h7xx_hal_pwr.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_rcc.o: $(HAL_SRC)/stm32h7xx_hal_rcc.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_tim.o: $(HAL_SRC)/stm32h7xx_hal_tim.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_uart.o: $(HAL_SRC)/stm32h7xx_hal_uart.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_rcc_ex.o: $(HAL_SRC)/stm32h7xx_hal_rcc_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_pwr_ex.o: $(HAL_SRC)/stm32h7xx_hal_pwr_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_uart_ex.o: $(HAL_SRC)/stm32h7xx_hal_uart_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_tim_ex.o: $(HAL_SRC)/stm32h7xx_hal_tim_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/hal_flash_ex.o: $(HAL_SRC)/stm32h7xx_hal_flash_ex.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/bsp_nucleo.o: $(BSP_SRC)/stm32h7xx_nucleo.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lan8742.o: $(LAN8742_SRC)/lan8742.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/lwip_api_lib.o: $(LWIP_API)/api_lib.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_api_msg.o: $(LWIP_API)/api_msg.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_err.o: $(LWIP_API)/err.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netbuf.o: $(LWIP_API)/netbuf.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netdb.o: $(LWIP_API)/netdb.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netifapi.o: $(LWIP_API)/netifapi.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sockets.o: $(LWIP_API)/sockets.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcpip.o: $(LWIP_API)/tcpip.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/lwip_def.o: $(LWIP_CORE)/def.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_dns.o: $(LWIP_CORE)/dns.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_inet_chksum.o: $(LWIP_CORE)/inet_chksum.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_init.o: $(LWIP_CORE)/init.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip.o: $(LWIP_CORE)/ip.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_mem.o: $(LWIP_CORE)/mem.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_memp.o: $(LWIP_CORE)/memp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_pbuf.o: $(LWIP_CORE)/pbuf.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_raw.o: $(LWIP_CORE)/raw.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_stats.o: $(LWIP_CORE)/stats.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sys.o: $(LWIP_CORE)/sys.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_altcp.o: $(LWIP_CORE)/altcp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_altcp_alloc.o: $(LWIP_CORE)/altcp_alloc.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_altcp_tcp.o: $(LWIP_CORE)/altcp_tcp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcp.o: $(LWIP_CORE)/tcp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcp_in.o: $(LWIP_CORE)/tcp_in.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_tcp_out.o: $(LWIP_CORE)/tcp_out.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_udp.o: $(LWIP_CORE)/udp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_netif.o: $(LWIP_CORE)/netif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip4.o: $(LWIP_IPV4)/ip4.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_icmp.o: $(LWIP_IPV4)/icmp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_etharp.o: $(LWIP_IPV4)/etharp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_dhcp.o: $(LWIP_IPV4)/dhcp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_acd.o: $(LWIP_IPV4)/acd.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_timeouts.o: $(LWIP_CORE)/timeouts.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip4_addr.o: $(LWIP_IPV4)/ip4_addr.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ip4_frag.o: $(LWIP_IPV4)/ip4_frag.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_ethernet.o: $(LWIP_NETIF)/ethernet.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_slip.o: $(LWIP_NETIF)/slipif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/lwip_sys_arch.o: $(LWIP_ARCH)/sys_arch.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/lwip.elf: $(OBJ) $(TOP)/app/lwip/lwip_nucleo_h743zi.ld
	$(LD) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
	@echo "Built: $(BUILD)/lwip.elf"

$(BUILD)/lwip.bin: $(BUILD)/lwip.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/lwip.bin (flash at 0x08000000)"

clean:
	rm -rf $(BUILD)

.PHONY: all clean
