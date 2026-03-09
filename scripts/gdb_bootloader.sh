#!/bin/bash
# Запуск GDB для отладки бутлоадера. Перед этим в другом терминале: st-util
# После подключения: (gdb) break main  (gdb) continue

cd "$(dirname "$0")/.."
ELF="${ELF:-build/bootloader/bootloader.elf}"

if [ ! -f "$ELF" ]; then
  echo "Соберите с отладочными символами: make -f bootloader/Makefile debug"
  exit 1
fi

arm-none-eabi-gdb -ex "target extended-remote localhost:4242" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  "$ELF"
