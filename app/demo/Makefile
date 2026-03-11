# Demo profile based on lwip_zero stack:
# FreeRTOS + LwIP + Netconn HTTP + UART log + I2C + USER button.
TOP ?= $(CURDIR)

all:
	$(MAKE) -f "$(TOP)/app/lwip_zero/Makefile" all \
		TOP="$(TOP)" \
		PROFILE=demo \
		APP_MAIN="$(TOP)/app/demo/main.c"

clean:
	$(MAKE) -f "$(TOP)/app/lwip_zero/Makefile" clean \
		TOP="$(TOP)" \
		PROFILE=demo \
		APP_MAIN="$(TOP)/app/demo/main.c"

.PHONY: all clean
