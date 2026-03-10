/*
 * Linker script for lwip_zero (STM32H743ZI).
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
    end = .;
  } > RAM_D1

  ._user_heap_stack : {
    . = ALIGN(8);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM_D1

  .lwip_sec (NOLOAD) : {
    . = ABSOLUTE(0x30000000);
    *(.RxDescripSection)

    . = ABSOLUTE(0x30000080);
    *(.TxDescripSection)

    . = ABSOLUTE(0x30000100);
    *(.Rx_PoolSection)
  } > RAM_D2

  .lwip_ram_heap 0x30004000 (NOLOAD) : {
    _lwip_ram_heap = .;
    . += 0x4000;
  } > RAM_D2
}
