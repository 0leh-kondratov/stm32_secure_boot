#!/bin/bash
# Прошивка Secure Boot в STM32H743 (NUCLEO-144).
# Подключите плату по USB и запустите: ./scripts/flash.sh
#
# На STM32H7 страница flash 128 KB — st-flash пишет только с границы страницы.
# Поэтому собираем один образ: бутлоадер (64 KB) + подписанное приложение.

set -e
cd "$(dirname "$0")/.."

echo "=== Проверка платы ==="
st-info --probe || { echo "Плата не найдена. Подключите NUCLEO-144 по USB."; exit 1; }

BOOTLOADER_SIZE=65536
COMBINED=$(mktemp -u).bin
trap "rm -f ${COMBINED}" EXIT

echo ""
echo "=== Сборка объединённого образа ==="
cp build/bootloader.bin "$COMBINED"
dd if=/dev/zero bs=1 count=$((BOOTLOADER_SIZE - $(wc -c < build/bootloader.bin))) 2>/dev/null >> "$COMBINED"
cat build_app/signed_app.bin >> "$COMBINED"

echo ""
echo "=== Прошивка (бутлоадер + приложение с 0x08000000) ==="
st-flash write "$COMBINED" 0x08000000

echo ""
echo "Готово. Нажмите Reset на плате — должен мигать зелёный LD1."
