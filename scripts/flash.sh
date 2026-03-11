#!/bin/bash
# Oprogramowanie sprzętowe bezpiecznego rozruchu w STM32H743 (NUCLEO-144).
# Podłącz płytkę przez USB i uruchom: ./scripts/flash.sh
#
# W STM32H7 strona flash ma 128 KB - st-flash zapisuje tylko od krawędzi strony.
# Dlatego składamy jeden obraz: bootloader (64 KB) + podpisana aplikacja.

set -e
cd "$(dirname "$0")/.."

echo "=== Tablica kontrolna ==="
st-info --sonda || { echo "Nie znaleziono płyty. Podłącz NUCLEO-144 przez USB."; wyjście 1; }

BOOTLOADER_SIZE=65536
COMBINED=$(mktemp -u).bin
trap "rm -f ${COMBINED}" EXIT

echo ""
echo "=== Montaż bootloadera ==="
make -f bootloader/Makefile all

echo ""
echo "===Budowanie i podpisywanie aplikacji ==="
make -f app/Makefile all
python3 scripts/sign_image.py build/app/app.bin build/app/signed_app.bin

echo ""
echo "===Tworzenie połączonego obrazu ==="
cp build/bootloader/bootloader.bin "$COMBINED"
dd if=/dev/zero bs=1 count=$((BOOTLOADER_SIZE - $(wc -c < build/bootloader/bootloader.bin))) 2>/dev/null >> "$COMBINED"
cat build/app/signed_app.bin >> "$COMBINED"

echo ""
echo "=== Firmware (bootloader + aplikacja z 0x08000000) ==="
st-flash write "$COMBINED" 0x08000000

echo ""
echo "Gotowe. Naciśnij Reset - 3 diody LED i dziennik UART (115200 8N1, USART3) powinien migać."
