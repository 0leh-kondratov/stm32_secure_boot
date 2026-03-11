#!/bin/bash
# Uruchom GDB, aby debugować bootloader. Wcześniej w innym terminalu: st-util
# Po połączeniu: (gdb) przerwa main (gdb) kontynuuj

cd "$(dirname "$0")/.."
ELF="${ELF:-build/bootloader/bootloader.elf}"

if [ ! -f "$ELF" ]; then
echo „Kompiluj z symbolami debugowania: make -f bootloader/debugowanie pliku Makefile”
  exit 1
fi

arm-none-eabi-gdb -ex "target extended-remote localhost:4242" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  "$ELF"
