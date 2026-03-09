# STM32H743 Secure Bootloader (NUCLEO-144)

Защищённый загрузчик с проверкой ECDSA P-256 подписи прошивки.

**Подробная инструкция по сборке и прошивке:** [docs/BUILD_RU.md](docs/BUILD_RU.md)

**Verified Boot на STM32 (терминология и концепции):** [docs/VERIFIED_BOOT_STM32_RU.md](docs/VERIFIED_BOOT_STM32_RU.md)

**Использование примеров для тестирования:** [docs/TESTING_RU.md](docs/TESTING_RU.md)

**Интерактивная отладка (GDB + st-util):** [docs/DEBUG_RU.md](docs/DEBUG_RU.md)

**Эмуляция в Renode (без платы):** [docs/SIMULATE_RU.md](docs/SIMULATE_RU.md)

**Что нужно для создания/прошивки образа под STM32 (наш проект и официальные ST):** [docs/WHAT_NEEDED_STM32_RU.md](docs/WHAT_NEEDED_STM32_RU.md)

**Быстрый старт X-CUBE-SBSFU (скачать и запустить на STM32):** [docs/SBSFU_QUICKSTART_RU.md](docs/SBSFU_QUICKSTART_RU.md)

Быстрый старт:
```bash
make
make -f app/Makefile all
source scripts/venv/bin/activate && python scripts/sign_image.py build/app/app.bin build/app/signed_app.bin
st-flash write build/bootloader/bootloader.bin 0x08000000
st-flash write build/app/signed_app.bin 0x08010000
```

--- СКОПИРУЙ ЭТО В СВОЙ C-КОД (bootloader/inc/keys.h) ---
const uint8_t root_public_key[] = {
    0x34, 0x86, 0x01, 0xda, 0x15, 0xea, 0x37, 0x48, 0xec, 0x40, 0x9c, 0xbd, 0x76, 0xc2, 0x98, 0x09, 0x29, 0x51, 0xec, 0xed, 0xcf, 0x66, 0xa4, 0x56, 0x41, 0x4e, 0xe0, 0x6a, 0x92, 0xf9, 0xf3, 0x3e, 0x8d, 0xc2, 0xd0, 0xed, 0x77, 0x55, 0x09, 0xfa, 0xa9, 0x92, 0x6f, 0xfa, 0x13, 0xac, 0xfe, 0x7c, 0x2e, 0x4b, 0x28, 0xb5, 0xc2, 0xdb, 0xda, 0xa4, 0x1d, 0x05, 0xcf, 0x41, 0x2e, 0x9a, 0x48, 0x42
};
---------------------------------------------------------
