/*
 * Memory map for STM32H743ZI (NUCLEO-144).
 * Flash: 0x08000000, 2 MB.
 * RAM:   0x20000000, 1 MB (DTCM optional).
 *
 * Layout:
 *   0x08000000 - 0x0800FFFF   Bootloader (64 KB)
 *   0x08010000 - ...          Application: image_header_t + code
 */
#ifndef COMMON_MEMORY_MAP_H
#define COMMON_MEMORY_MAP_H

#define FLASH_BASE             0x08000000UL
#define BOOTLOADER_BASE        0x08000000UL
#define BOOTLOADER_SIZE        (64U * 1024U)

#define IMAGE_HEADER_ADDRESS   0x08010000UL
#define APP_BASE               0x08010000UL   /* Same as header; code starts after header */
/* APP_START_ADDRESS = IMAGE_HEADER_ADDRESS + sizeof(image_header_t) is computed in C */

#endif /* COMMON_MEMORY_MAP_H */
