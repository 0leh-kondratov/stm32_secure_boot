#!/bin/bash
# Debugowanie aplikacji/lwip na płycie NUCLEO-H743ZI2 (STM32H743).
# Przed uruchomieniem w innym terminalu: st-util
# W GDB: kontynuuj - zatrzymaj się na głównym; przerwa StartDefaultTask itp.
# Potrzebujesz arm-none-eabi-gdb (nie systemowego gdb dla x86).

BOARD="NUCLEO-H743ZI2"
cd "$(dirname "$0")/.."
ELF="${ELF:-build/lwip/lwip.elf}"
GDB="${GDB:-arm-none-eabi-gdb}"

if ! command -v "$GDB" &>/dev/null; then
echo „Błąd: nie znaleziono $GDB. Zainstaluj: sudo apt install gcc-arm-none-eabi”
  exit 1
fi

if [ ! -f "$ELF" ]; then
echo "Zbuduj obraz: make lwip"
  exit 1
fi

echo "Płyta: $BOARD (STM32H743)"
echo "GDB:   $GDB ($(command -v "$GDB"))"
echo "ELF:   $ELF"
echo "---"

# Po uderzeniu HardFault: dochodzimy do while(1), drukujemy HFSR/CFSR/BFAR/PC i zatrzymujemy się.
GDBINIT=$(mktemp)
trap "rm -f $GDBINIT" EXIT
cat >> "$GDBINIT" << 'GDBEOF'
break HardFault_Handler
commands
silent
until 66
printf "--- HardFault ---\n"
printf "HFSR=0x%x  CFSR=0x%x  BFAR=0x%x\n", g_hardfault_hfsr, g_hardfault_cfsr, g_hardfault_bfar
printf "PC (faulting)=0x%x  ", g_hardfault_pc
x/i g_hardfault_pc
printf "--- (stop: możesz spojrzeć na zmienne, a następnie c, aby powtórzyć)\n"
end
GDBEOF

"$GDB" -x "$GDBINIT" -ex "target extended-remote localhost:4242" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  "$ELF"
