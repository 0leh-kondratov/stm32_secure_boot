#!/bin/bash
# Sprawdzanie montażu wszystkich celów bez oprogramowania sprzętowego.
# Uruchom: ./scripts/build_all.sh
# Wyjście: 0 - wszystko zadziałało, w przeciwnym razie pierwszy kod błędu.

set -e
cd "$(dirname "$0")/.."

echo "=== clean ==="
make -f app/demo/Makefile clean 2>/dev/null || true
make -f bootloader/Makefile clean

echo ""
echo "=== bootloader ==="
make -f bootloader/Makefile all

echo ""
echo "=== app + signed-app ==="
make -f app/Makefile all
python3 scripts/sign_image.py build/app/app.bin build/app/signed_app.bin

echo ""
echo "=== demo ==="
make -f app/demo/Makefile all

echo ""
echo "=== minimal-usart3 ==="
make -f bootloader/Makefile minimal-usart3

echo ""
echo "=== build_all: OK ==="
