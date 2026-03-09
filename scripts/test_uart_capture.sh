#!/bin/bash
# Тест вывода минимального бутлоадера в UART.
# Закройте minicom. Запустите: ./scripts/test_uart_capture.sh
# По запросу нажмите Reset на плате и сразу Enter — скрипт покажет захваченный вывод.

set -e
cd "$(dirname "$0")/.."

DEV="${1:-/dev/ttyACM0}"
echo "Port: $DEV (115200 8N1)"
echo ""

if ! [ -e "$DEV" ]; then
    echo "Error: $DEV not found. Connect the board."
    exit 1
fi

# Ensure minimal bootloader is built
if ! [ -f build/bootloader_minimal.bin ]; then
    echo "Building minimal bootloader..."
    make minimal
fi

stty -F "$DEV" 115200 raw -echo 2>/dev/null || { echo "Cannot set $DEV (close minicom?)"; exit 1; }

echo "Capture will start. Press RESET on the board NOW, then press Enter here to stop."
( timeout 10 cat "$DEV" > /tmp/uart_capture.txt 2>/dev/null ) &
CATPID=$!
read -r
kill $CATPID 2>/dev/null
wait $CATPID 2>/dev/null
sleep 0.2
echo ""
echo "=== Captured ($(wc -c < /tmp/uart_capture.txt) bytes) ==="
cat -v /tmp/uart_capture.txt
echo ""
