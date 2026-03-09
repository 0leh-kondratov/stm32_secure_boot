#!/bin/bash
# Проверка сборки всех целей без прошивки.
# Запуск: ./scripts/build_all.sh
# Выход: 0 — всё собралось, иначе первый код ошибки.

set -e
cd "$(dirname "$0")/.."

echo "=== clean ==="
make -f demo/Makefile clean 2>/dev/null || true
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
make -f demo/Makefile all

echo ""
echo "=== minimal-usart3 ==="
make -f bootloader/Makefile minimal-usart3

echo ""
echo "=== build_all: OK ==="
