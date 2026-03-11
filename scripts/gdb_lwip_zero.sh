#!/bin/bash
# Debugowanie app/lwip_zero na płycie NUCLEO-H743ZI2 (STM32H743).
# Przed uruchomieniem w innym terminalu: st-util
# Potrzebujesz arm-none-eabi-gdb (nie systemowego gdb dla x86).

BOARD="NUCLEO-H743ZI2"
cd "$(dirname "$0")/.."
ELF="${ELF:-build/lwip_zero/lwip_zero.elf}"
GDB="${GDB:-arm-none-eabi-gdb}"

if ! command -v "$GDB" &>/dev/null; then
echo „Błąd: nie znaleziono $GDB. Zainstaluj: sudo apt install gcc-arm-none-eabi”
  exit 1
fi

if [ ! -f "$ELF" ]; then
echo "Zbuduj obraz: zrób lwip-zero"
  exit 1
fi

echo "Płyta: $BOARD (STM32H743)"
echo "GDB:   $GDB ($(command -v "$GDB"))"
echo "ELF:   $ELF"
echo "---"

GDBINIT=$(mktemp)
trap "rm -f $GDBINIT" EXIT

cat >> "$GDBINIT" << 'GDBEOF'
set pagination off
set confirm off

break HardFault_Handler
commands
silent
printf "--- HardFault_Handler ---\n"
info registers
set $cfsr = *(unsigned int *)0xE000ED28
set $hfsr = *(unsigned int *)0xE000ED2C
set $bfar = *(unsigned int *)0xE000ED38
set $mmfar = *(unsigned int *)0xE000ED34
if (($lr & 4) != 0)
  set $fault_sp = $psp
else
  set $fault_sp = $msp
end
set $fault_r0 = *(unsigned int *)($fault_sp + 0)
set $fault_r1 = *(unsigned int *)($fault_sp + 4)
set $fault_r2 = *(unsigned int *)($fault_sp + 8)
set $fault_r3 = *(unsigned int *)($fault_sp + 12)
set $fault_r12 = *(unsigned int *)($fault_sp + 16)
set $fault_lr = *(unsigned int *)($fault_sp + 20)
set $fault_pc = *(unsigned int *)($fault_sp + 24)
set $fault_xpsr = *(unsigned int *)($fault_sp + 28)
printf "CFSR=0x%08x HFSR=0x%08x BFAR=0x%08x MMFAR=0x%08x\n", $cfsr, $hfsr, $bfar, $mmfar
printf "fault_sp=0x%08x fault_lr=0x%08x fault_pc=0x%08x fault_xpsr=0x%08x\n", $fault_sp, $fault_lr, $fault_pc, $fault_xpsr
printf "stacked: r0=0x%08x r1=0x%08x r2=0x%08x r3=0x%08x r12=0x%08x\n", $fault_r0, $fault_r1, $fault_r2, $fault_r3, $fault_r12
x/i $fault_pc
x/8wx $fault_sp
backtrace
printf "--- (zatrzymaj w HardFault, zobacz rejestry/stos)\n"
end

break UsageFault_Handler
commands
silent
printf "--- UsageFault_Handler ---\n"
info registers
backtrace
printf "--- (zatrzymany w UsageFault, zobacz rejestry/stos)\n"
end
GDBEOF

"$GDB" -x "$GDBINIT" \
  -ex "target extended-remote localhost:4242" \
  -ex "monitor reset halt" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  "$ELF"
