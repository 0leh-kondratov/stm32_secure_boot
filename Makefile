# Top-level Makefile. Run from project root: make
# Builds bootloader by default.

# Path to STM32CubeH7 (working official Templates). Override: make CUBE_ROOT=/path
CUBE_ROOT ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../STM32CubeH7)

all: bootloader

bootloader:
	$(MAKE) -f bootloader/Makefile all

app:
	$(MAKE) -f app/Makefile all

# Signed app for secure boot (requires scripts/venv or system ecdsa)
signed-app: app
	@mkdir -p build/app
	python3 scripts/sign_image.py build/app/app.bin build/app/signed_app.bin

minimal:
	$(MAKE) -f bootloader/Makefile minimal

# Сборка бутлоадера с отладочными символами для GDB (-g -O0)
debug:
	$(MAKE) -f bootloader/Makefile debug

demo:
	$(MAKE) -f app/demo/Makefile all CUBE_ROOT="$(CUBE_ROOT)"
# Demo для Renode: автокоманды раз в 5 с (обход бага UART RX в эмуляторе)
demo-renode:
	$(MAKE) -f app/demo/Makefile renode CUBE_ROOT="$(CUBE_ROOT)" TOP="$(CURDIR)"

# LwIP + FreeRTOS + TCP echo server (NUCLEO-H743ZI). Requires CUBE_ROOT with Drivers and Middlewares.
lwip:
	$(MAKE) -f app/lwip/Makefile all CUBE_ROOT="$(CUBE_ROOT)"

# New clean firmware profile: FreeRTOS + LwIP from scratch.
lwip-zero:
	$(MAKE) -f app/lwip_zero/Makefile all

# Stop st-util (and GDB) before flashing — ST-Link can only be used by one tool at a time.
flash-lwip: lwip
	st-flash write build/lwip/lwip.bin 0x08000000 || (echo "Hint: stop st-util (Ctrl+C in that terminal) and retry"; exit 255)

flash-lwip-zero: lwip-zero
	st-flash write build/lwip_zero/lwip_zero.bin 0x08000000 || (echo "Hint: stop st-util (Ctrl+C in that terminal) and retry"; exit 255)

# Прошить standalone demo (FreeRTOS, LED+UART) в 0x08000000
flash-demo: demo
	st-flash write build/demo/demo.bin 0x08000000

# Working official STM32CubeH7 Templates (LEDs blink). Build from Cube repo.
demo-official:
	@if [ ! -d "$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates" ]; then \
		echo "Error: STM32CubeH7 not found at $(CUBE_ROOT)"; \
		echo "Clone it or set CUBE_ROOT=..."; exit 1; fi
	$(MAKE) -C "$(CUBE_ROOT)" -f Projects/NUCLEO-H743ZI/Templates/Makefile all
	@echo "Built: $(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/build/Templates.bin"

# Flash the official demo (run after make demo-official)
flash-demo-official: demo-official
	st-flash write "$(CUBE_ROOT)/Projects/NUCLEO-H743ZI/Templates/build/Templates.bin" 0x08000000

# Step 1: FreeRTOS Logger + LED + App. Для платы — пересобрать main.o, startup.o, system (без STEP1_RENODE).
step1:
	@rm -f build/step1/main.o build/step1/startup.o build/step1/system_stm32h7xx.o
	$(MAKE) -f app/step1/Makefile all CUBE_ROOT="$(CUBE_ROOT)"

flash-step1: step1
	st-flash write build/step1/step1.bin 0x08000000

# Полная очистка Flash, затем прошивка step1 (плата в начальное состояние)
erase-flash-step1: step1
	st-flash erase
	st-flash write build/step1/step1.bin 0x08000000

# Проверка платы: UART должен выводить "Step 1: OK". Порт: make test-board-step1 PORT=/dev/ttyUSB0
test-board-step1:
	@$(if $(PORT),./scripts/test_board_step1.sh $(PORT),./scripts/test_board_step1.sh)

# То же с ручным сбросом (нажать Reset по запросу) — если после st-info порт не отдаёт данные
test-board-step1-no-reset:
	./scripts/test_board_step1.sh --no-reset

# С прошивкой перед проверкой: make test-board-step1-flash
test-board-step1-flash:
	./scripts/test_board_step1.sh --flash

# Step1 в Renode: сборка образа для эмуляции (HSI 64 MHz, без HSE) + подсказка запуска
step1-renode:
	$(MAKE) -f app/step1/Makefile renode CUBE_ROOT="$(CUBE_ROOT)" TOP="$(CURDIR)"
	@echo "Run: renode step1.resc"
	@echo "  UART: usart3 / telnet localhost 12345 — ожидается \"Step 1: OK\" каждые 2 с."

# Step 2: FreeRTOS + Logger + LED + I2C scanner (scan result to UART log).
step2:
	$(MAKE) -f app/step2/Makefile all CUBE_ROOT="$(CUBE_ROOT)"

flash-step2: step2
	st-flash write build/step2/step2.bin 0x08000000

# Stage 1: I2C Scanner (no FreeRTOS). Result: address 0x27 in UART.
test-stage1:
	$(MAKE) -f test/stage1_i2c_scanner/Makefile all CUBE_ROOT="$(CUBE_ROOT)"

# Stage 2: FreeRTOS + LCD counter every 1s, "Wallet Init" on screen.
test-stage2:
	$(MAKE) -f test/stage2_freertos_lcd/Makefile all CUBE_ROOT="$(CUBE_ROOT)"

flash-stage1: test-stage1
	st-flash write build/stage1/stage1.bin 0x08000000

flash-stage2: test-stage2
	st-flash write build/stage2/stage2.bin 0x08000000

clean: clean-demo clean-lwip clean-tests clean-step1 clean-step2
	$(MAKE) -f bootloader/Makefile clean

clean-step1:
	rm -rf build/step1

clean-step2:
	$(MAKE) -f app/step2/Makefile clean 2>/dev/null || true
	rm -rf build/step2

clean-demo:
	$(MAKE) -f app/demo/Makefile clean 2>/dev/null || true

clean-lwip:
	$(MAKE) -f app/lwip/Makefile clean 2>/dev/null || true

clean-lwip-zero:
	$(MAKE) -f app/lwip_zero/Makefile clean 2>/dev/null || true

clean-tests:
	rm -rf build/stage1 build/stage2

.PHONY: all bootloader app signed-app minimal debug demo demo-renode flash-demo demo-official flash-demo-official lwip lwip-zero flash-lwip flash-lwip-zero step1 flash-step1 erase-flash-step1 test-board-step1 test-board-step1-no-reset test-board-step1-flash step1-renode step2 flash-step2 test-stage1 test-stage2 flash-stage1 flash-stage2 clean clean-demo clean-lwip clean-lwip-zero clean-tests clean-step1 clean-step2
