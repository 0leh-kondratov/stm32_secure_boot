# Top-level Makefile. Run from project root: make
# Builds bootloader by default.

all: bootloader

bootloader:
	$(MAKE) -f bootloader/Makefile all

clean:
	$(MAKE) -f bootloader/Makefile clean

.PHONY: all bootloader clean
