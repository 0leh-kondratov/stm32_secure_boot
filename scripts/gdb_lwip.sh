#!/bin/bash
# Отладка app/lwip на плате NUCLEO-H743ZI2 (STM32H743).
# Перед запуском в другом терминале: st-util
# В GDB: continue — остановка на main; break StartDefaultTask и т.д.
# Нужен arm-none-eabi-gdb (не системный gdb для x86).

BOARD="NUCLEO-H743ZI2"
cd "$(dirname "$0")/.."
ELF="${ELF:-build/lwip/lwip.elf}"
GDB="${GDB:-arm-none-eabi-gdb}"

if ! command -v "$GDB" &>/dev/null; then
  echo "Ошибка: $GDB не найден. Установите: sudo apt install gcc-arm-none-eabi"
  exit 1
fi

if [ ! -f "$ELF" ]; then
  echo "Соберите образ: make lwip"
  exit 1
fi

echo "Плата: $BOARD (STM32H743)"
echo "GDB:   $GDB ($(command -v "$GDB"))"
echo "ELF:   $ELF"
echo "---"

# При попадании в HardFault: доходим до while(1), выводим HFSR/CFSR/BFAR/PC и останавливаемся.
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
printf "--- (остановка: можно смотреть переменные, затем c для повтора)\n"
end
GDBEOF

"$GDB" -x "$GDBINIT" -ex "target extended-remote localhost:4242" \
  -ex "load" \
  -ex "monitor reset halt" \
  -ex "break main" \
  "$ELF"
