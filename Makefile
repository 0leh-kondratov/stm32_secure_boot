# Top-level Makefile. Run from project root: make
# Builds bootloader by default.

# Path to STM32CubeH7 (working official Templates). Override: make CUBE_ROOT=/path
CUBE_ROOT ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../STM32CubeH7)

all: bootloader

bootloader:
	$(MAKE) -f bootloader/Makefile all

minimal:
	$(MAKE) -f bootloader/Makefile minimal

demo:
	$(MAKE) -f demo/Makefile all

# Working official STM32CubeH7 Templates (LEDs blink). Build from Cube repo.
demo-official:
	@if [ ! -d "$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates" ]; then \
		echo "Error: STM32CubeH7 not found at $(CUBE_ROOT)"; \
		echo "Clone it or set CUBE_ROOT=..."; exit 1; fi
	$(MAKE) -C "$(CUBE_ROOT)" -f Projects/NUCLEO-H743ZI/Templates/Makefile all
	@echo "Built: $(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/build/Templates.bin"

# Flash the official demo (run after make demo-official)
flash-demo-official: demo-official
	st-flash write "$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/build/Templates.bin" 0x08000000

clean: clean-demo
	$(MAKE) -f bootloader/Makefile clean

clean-demo:
	$(MAKE) -f demo/Makefile clean 2>/dev/null || true

.PHONY: all bootloader minimal demo demo-official flash-demo-official clean clean-demo
