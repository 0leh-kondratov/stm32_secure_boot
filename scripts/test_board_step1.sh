#!/bin/bash
# Проверка работоспособности платы NUCLEO-H743ZI с прошивкой step1.
# Ожидается: по UART каждые 2 с строка "Step 1: OK".
#
# Использование:
#   ./scripts/test_board_step1.sh              # плата уже прошита, порт /dev/ttyACM0
#   ./scripts/test_board_step1.sh /dev/ttyUSB0
#   ./scripts/test_board_step1.sh --flash      # сначала прошить, затем проверить
#   ./scripts/test_board_step1.sh --no-reset   # не сбрасывать по ST-Link — нажать Reset вручную
#
# Перед запуском закройте minicom/screen на том же порту.

set -e
cd "$(dirname "$0")/.."

DO_FLASH=""
NO_RESET=""
DEV="/dev/ttyACM0"
for arg in "$@"; do
  if [ "$arg" = "--flash" ]; then
    DO_FLASH=1
  elif [ "$arg" = "--no-reset" ]; then
    NO_RESET=1
  elif [ -n "$arg" ] && [ "${arg#--}" = "$arg" ]; then
    DEV="$arg"
  fi
done

echo "=== Test board step1 (NUCLEO-H743ZI) ==="
echo "Port: $DEV (115200 8N1)"
echo ""

if [ -n "$DO_FLASH" ]; then
  echo "Flashing step1..."
  make erase-flash-step1
  echo ""
fi

if ! [ -e "$DEV" ]; then
  echo "Error: $DEV not found. Connect the board (ST-Link USB)."
  exit 1
fi

if ! [ -f build/step1/step1.bin ]; then
  echo "Error: build/step1/step1.bin not found. Run: make step1"
  exit 1
fi

# Порт не должен быть занят другим процессом
if command -v lsof >/dev/null 2>&1; then
  if lsof "$DEV" 2>/dev/null | grep -q .; then
    echo "Warning: $DEV is open by another process (close minicom/screen/ide):"
    lsof "$DEV" 2>/dev/null | head -3
    echo ""
  fi
fi

stty -F "$DEV" 115200 raw -echo 2>/dev/null || { echo "Error: cannot set $DEV (close minicom/screen?)"; exit 1; }

# Сброс платы через ST-Link (если есть st-info) или вручную
if [ -n "$NO_RESET" ]; then
  echo "Press RESET on the board, then press Enter here."
  read -r
  sleep 0.5
elif command -v st-info >/dev/null 2>&1; then
  echo "Resetting board..."
  st-info --reset 2>/dev/null || true
  echo "Waiting 3 s for USB and firmware..."
  sleep 3
  # После сброса порт может переподключиться — проверить снова
  if ! [ -e "$DEV" ]; then
    echo "Error: $DEV disappeared after reset (USB re-enumeration?). Try: make test-board-step1 PORT=/dev/ttyACM1"
    echo "Or run: make test-board-step1 --no-reset  (then press RESET when asked)"
    exit 1
  fi
  stty -F "$DEV" 115200 raw -echo 2>/dev/null || true
else
  echo "Press RESET on the board, then press Enter here."
  read -r
  sleep 0.5
fi

echo "Capturing UART for 10 s..."
CAPTURE="/tmp/step1_uart_capture_$$.txt"
timeout 10 cat "$DEV" > "$CAPTURE" 2>/dev/null || true

if grep -q "Step 1: OK" "$CAPTURE" 2>/dev/null; then
  echo ""
  echo "*** Board test PASS: 'Step 1: OK' received. ***"
  rm -f "$CAPTURE"
  exit 0
else
  echo ""
  echo "*** Board test FAIL: no 'Step 1: OK' in capture. ***"
  echo "Captured ($(wc -c < "$CAPTURE" 2>/dev/null || echo 0) bytes):"
  cat -v "$CAPTURE" 2>/dev/null | head -50
  echo ""
  echo "Diagnostics (0 bytes = no UART data):"
  echo "  Available ports (try another):"
  ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true
  echo "  - Check cable is on ST-Link USB (not the other USB connector on NUCLEO)."
  echo "  - In minicom: minicom -D $DEV -b 115200, press Reset. See 'Step 1: OK'?"
  echo "    If yes in minicom but test fails → try: make test-board-step1-no-reset PORT=/dev/ttyACM1"
  echo "    If no in minicom → reflash: make step1 && make erase-flash-step1. If still no, try RCC_HSE_BYPASS in app/step1/main.c (HSEState) when HSE is from ST-Link MCO."
  rm -f "$CAPTURE"
  exit 1
fi
