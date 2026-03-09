/*
 * Linker script for LwIP + FreeRTOS on NUCLEO-H743ZI2 (STM32H743ZI).
 * Memory: DTCM 128K, RAM_D1 512K, RAM_D2 288K (D2 domain for ETH DMA and LwIP).
 * Sections .RxDecriptorSection, .TxDecriptorSection and .LwipSection are placed
 * in RAM_D2 so the MPU can mark this region non-cacheable for Ethernet DMA
 * (descriptors and buffers must be coherent with the ETH DMA engine).
 */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

_estack = 0x24000000 + 512K;
_Min_Heap_Size = 0x400;
_Min_Stack_Size = 0x800;

MEMORY
{
  ROM (rx)       : ORIGIN = 0x08000000, LENGTH = 2048K
  DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (xrw)   : ORIGIN = 0x24000000, LENGTH = 512K
  RAM_D2 (xrw)   : ORIGIN = 0x30000000, LENGTH = 288K
}

SECTIONS
{
  .isr_vector : {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > ROM

  .text : {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } > ROM

  .data : {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM_D1 AT > ROM
  _sidata = LOADADDR(.data);

  .bss : {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    _end = .;
  } > RAM_D1

  ._user_heap_stack : {
    . = ALIGN(8);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM_D1

  /* Ethernet DMA descriptors and LwIP buffers in D2 SRAM (non-cacheable via MPU).
   * ETH DMA must access these without cache coherency issues. */
  .RxDecriptorSection (NOLOAD) : {
    . = ALIGN(4);
    *(.RxDecriptorSection)
  } > RAM_D2

  .TxDecriptorSection (NOLOAD) : {
    . = ALIGN(4);
    *(.TxDecriptorSection)
  } > RAM_D2

  .LwipSection (NOLOAD) : {
    . = ALIGN(4);
    *(.LwipSection)
    *(.Rx_PoolSection)
    . = ALIGN(4);
  } > RAM_D2
}
