# lwip_zero: clean FreeRTOS + LwIP firmware build (NUCLEO-H743ZI2).

TOP       ?= $(CURDIR)
CUBE_LOCAL ?= $(TOP)/app/lwip_zero/cube
APP_LOCAL  ?= $(TOP)/app/lwip_zero/cube_project

CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
AS      = arm-none-eabi-gcc -x assembler-with-cpp

CFLAGS  = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -std=c11 -Wall -O2 -g
CFLAGS += -DDEBUG -DSTM32H743xx -DUSE_HAL_DRIVER -DUSE_PWR_LDO_SUPPLY -DUSE_STM32H7XX_NUCLEO_144_MB1364
CFLAGS += -DUART_LOG_USE_FREERTOS_QUEUE
CFLAGS += -I$(TOP)/app/lwip_zero/Inc
CFLAGS += -I$(TOP)/common
CFLAGS += -I$(APP_LOCAL)/Inc
CFLAGS += -I$(CUBE_LOCAL)/Drivers/CMSIS/Include
CFLAGS += -I$(CUBE_LOCAL)/Drivers/CMSIS/Device/ST/STM32H7xx/Include
CFLAGS += -I$(CUBE_LOCAL)/Drivers/CMSIS/RTOS2/Include
CFLAGS += -I$(CUBE_LOCAL)/Drivers/STM32H7xx_HAL_Driver/Inc
CFLAGS += -I$(CUBE_LOCAL)/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
CFLAGS += -I$(CUBE_LOCAL)/Drivers/BSP/STM32H7xx_Nucleo
CFLAGS += -I$(CUBE_LOCAL)/Drivers/BSP/Components/Common
CFLAGS += -I$(CUBE_LOCAL)/Drivers/BSP/Components/lan8742
CFLAGS += -I$(CUBE_LOCAL)/Middlewares/Third_Party/LwIP/src/include
CFLAGS += -I$(CUBE_LOCAL)/Middlewares/Third_Party/LwIP/system
CFLAGS += -I$(CUBE_LOCAL)/Middlewares/Third_Party/FreeRTOS/Source/include
CFLAGS += -I$(CUBE_LOCAL)/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
CFLAGS += -I$(CUBE_LOCAL)/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F

LDFLAGS = -T$(TOP)/app/lwip_zero/lwip_zero_h743zi.ld -nostdlib -Wl,--gc-sections -lgcc -specs=nosys.specs -lc -lnosys

BUILD   = build/lwip_zero
APP_SRC = $(APP_LOCAL)/Src
FREERTOS_CUBE = $(CUBE_LOCAL)/Middlewares/Third_Party/FreeRTOS/Source
PORT    = $(FREERTOS_CUBE)/portable/GCC/ARM_CM4F
MEMMANG = $(FREERTOS_CUBE)/portable/MemMang
HAL_SRC = $(CUBE_LOCAL)/Drivers/STM32H7xx_HAL_Driver/Src
BSP_SRC = $(CUBE_LOCAL)/Drivers/BSP/STM32H7xx_Nucleo
LAN8742_SRC = $(CUBE_LOCAL)/Drivers/BSP/Components/lan8742
LWIP_SRC = $(CUBE_LOCAL)/Middlewares/Third_Party/LwIP/src
LWIP_API = $(LWIP_SRC)/api
LWIP_CORE = $(LWIP_SRC)/core
LWIP_NETIF = $(LWIP_SRC)/netif
LWIP_APPS = $(LWIP_SRC)/apps
LWIP_SYS = $(CUBE_LOCAL)/Middlewares/Third_Party/LwIP/system
LWIP_ARCH = $(LWIP_SYS)/arch
LWIP_IPV4 = $(LWIP_SRC)/core/ipv4
LWIPOPTS_HDR = $(TOP)/app/lwip_zero/Inc/lwipopts.h
STARTUP = $(CUBE_LOCAL)/Drivers/CMSIS/Device/ST/STM32H7xx/Source/Templates/gcc/startup_stm32h743xx.s

OBJ = $(BUILD)/startup.o \
      $(BUILD)/main.o $(BUILD)/netconn_page.o $(BUILD)/time_service.o $(BUILD)/uart_log.o $(BUILD)/ethernetif.o $(BUILD)/app_ethernet.o \
      $(BUILD)/stm32h7xx_it.o $(BUILD)/system_stm32h7xx.o $(BUILD)/stm32h7xx_hal_timebase_tim.o \
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
      $(BUILD)/lwip_tcp.o $(BUILD)/lwip_tcp_in.o $(BUILD)/lwip_tcp_out.o $(BUILD)/lwip_udp.o $(BUILD)/lwip_netif.o \
      $(BUILD)/lwip_ip4.o $(BUILD)/lwip_etharp.o $(BUILD)/lwip_icmp.o $(BUILD)/lwip_dhcp.o $(BUILD)/lwip_acd.o $(BUILD)/lwip_timeouts.o \
      $(BUILD)/lwip_ip4_addr.o $(BUILD)/lwip_ip4_frag.o \
      $(BUILD)/lwip_ethernet.o $(BUILD)/lwip_slip.o $(BUILD)/lwip_sys_arch.o $(BUILD)/lwip_sntp.o

all: $(BUILD)/lwip_zero.elf $(BUILD)/lwip_zero.bin

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	$(AS) $(CFLAGS) -c -o $@ $<

$(BUILD)/main.o: $(TOP)/app/lwip_zero/main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/netconn_page.o: $(TOP)/app/lwip_zero/netconn_page.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/time_service.o: $(TOP)/app/lwip_zero/time_service.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/uart_log.o: $(TOP)/common/uart_log.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/ethernetif.o: $(APP_SRC)/ethernetif.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/app_ethernet.o: $(APP_SRC)/app_ethernet.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_it.o: $(APP_SRC)/stm32h7xx_it.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/system_stm32h7xx.o: $(APP_SRC)/system_stm32h7xx.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/stm32h7xx_hal_timebase_tim.o: $(APP_SRC)/stm32h7xx_hal_timebase_tim.c | $(BUILD)
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
$(BUILD)/lwip_timeouts.o: $(LWIP_CORE)/timeouts.c | $(BUILD)
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
$(BUILD)/lwip_sntp.o: $(LWIP_APPS)/sntp/sntp.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/lwip_%.o: $(LWIPOPTS_HDR)

$(BUILD)/lwip_zero.elf: $(OBJ) $(TOP)/app/lwip_zero/lwip_zero_h743zi.ld
	$(LD) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
	@echo "Built: $(BUILD)/lwip_zero.elf"

$(BUILD)/lwip_zero.bin: $(BUILD)/lwip_zero.elf
	$(OBJCOPY) -O binary $< $@
	@echo "Built: $(BUILD)/lwip_zero.bin (flash at 0x08000000)"

clean:
	rm -rf $(BUILD)

.PHONY: all clean
