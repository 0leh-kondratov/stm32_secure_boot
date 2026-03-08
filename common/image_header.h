// Firmware image header description used by secure bootloader and image generator tool.
// All comments are kept in English to match project requirements.

#ifndef COMMON_IMAGE_HEADER_H
#define COMMON_IMAGE_HEADER_H

#include <stdint.h>

typedef struct
{
    uint32_t magic;        // Magic constant identifying a valid image
    uint32_t version;      // Firmware version number
    uint32_t image_size;   // Application image size in bytes (excluding this header)
    uint32_t entry_point;  // Application entry address
    uint8_t  reserved[16]; // Reserved for future use / alignment
    uint8_t  signature[64];// ECDSA P-256 signature: r(32) || s(32), big-endian
} image_header_t;

#endif // COMMON_IMAGE_HEADER_H

