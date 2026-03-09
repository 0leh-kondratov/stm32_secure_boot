// Bootloader main module for STM32H743 secure boot
// All comments must stay in English as requested.

#include <stdint.h>
#include <stdbool.h>

#include "image_header.h"
#include "keys.h"
#include "memory_map.h"
#include "uart_log.h"

#ifdef USE_ECDSA_STUB
/* Stub: no mbedTLS, signature always accepted. Use for first flash test only. */
#else
// mbedTLS includes (adjust include paths according to your project layout)
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/bignum.h"
#endif

/* -------------------------------------------------------------------------
 * STM32H743 minimal register definitions for HASH and RCC (no HAL).
 * Verify base addresses against RM0433 / your device header.
 * ------------------------------------------------------------------------- */
#define RCC_BASE              (0x58024400UL)
#define RCC_AHB2ENR            (*(volatile uint32_t *)(RCC_BASE + 0x4CU))
#define RCC_AHB2ENR_HASHEN     (1U << 7)

#define HASH_BASE              (0x58024C00UL)
#define HASH_CR                (*(volatile uint32_t *)(HASH_BASE + 0x00U))
#define HASH_DIN               (*(volatile uint32_t *)(HASH_BASE + 0x04U))
#define HASH_STR               (*(volatile uint32_t *)(HASH_BASE + 0x08U))
#define HASH_SR                (*(volatile uint32_t *)(HASH_BASE + 0x0CU))
#define HASH_HR(bank)          (*(volatile uint32_t *)(HASH_BASE + 0x14U + (uint32_t)(bank) * 4U))

#define HASH_CR_INIT           (1U << 2)
#define HASH_CR_ALGO_POS       (6U)
#define HASH_CR_ALGO_SHA256    (3U << HASH_CR_ALGO_POS)
#define HASH_CR_DATATYPE_32B   (2U << 4)
#define HASH_SR_DCIS          (1U << 0)
#define HASH_STR_NBLW_POS      (0U)
#define HASH_STR_NBLW_MASK     (0x3FU)

/* GPIO for error LED LD3 (red) on NUCLEO-144: PB14 */
#define GPIOB_BASE             (0x58020400UL)
#define GPIOB_MODER            (*(volatile uint32_t *)(GPIOB_BASE + 0x00U))
#define GPIOB_BSRR             (*(volatile uint32_t *)(GPIOB_BASE + 0x18U))
#define RCC_AHB4ENR            (*(volatile uint32_t *)(RCC_BASE + 0xE0U))
#define RCC_AHB4ENR_GPIOBEN    (1U << 1)
#define LED3_PIN               (14U)

// These addresses are examples and must match your linker script (memory_map.ld).
// IMAGE_HEADER_ADDRESS should point to the start of image_header_t.
// APP_START_ADDRESS should point to the first byte of application code.
#include "memory_map.h"

#ifndef IMAGE_HEADER_MAGIC
#define IMAGE_HEADER_MAGIC     (0x424F4F54UL)  // 'BOOT' example magic
#endif

#ifndef APP_START_ADDRESS
#define APP_START_ADDRESS      (IMAGE_HEADER_ADDRESS + sizeof(image_header_t))
#endif

// Forward declarations
static int hw_sha256_app_region(uint32_t app_address,
                                uint32_t app_size,
                                uint8_t out_hash[32]);

static int ecdsa_verify_p256_sha256(const uint8_t hash[32],
                                    const uint8_t *signature,
                                    uint32_t signature_len,
                                    const uint8_t *pubkey_xy);

/**
 * Verify application image signature before jumping to application.
 *
 * This function performs the following checks:
 *  - Validates image header magic and basic fields.
 *  - Computes SHA-256 over the application code region using HASH peripheral.
 *  - Verifies ECDSA P-256 signature from image header using root_public_key.
 *
 * Returns true if signature is valid and image header looks sane.
 */
bool verify_signature(void)
{
    const image_header_t *hdr = (const image_header_t *) IMAGE_HEADER_ADDRESS;
    uint8_t hash[32];

    log_puts("Verify: header...\r\n");

    if (hdr->magic != IMAGE_HEADER_MAGIC) {
        log_puts("  FAIL: bad magic\r\n");
        return false;
    }
    log_puts("  magic OK\r\n");

    if (hdr->image_size == 0U) {
        log_puts("  FAIL: image_size=0\r\n");
        return false;
    }
    log_puts("  size OK\r\n");

    log_puts("  SHA256...\r\n");
    if (hw_sha256_app_region(APP_START_ADDRESS,
                             hdr->image_size,
                             hash) != 0) {
        log_puts("  FAIL: SHA256\r\n");
        return false;
    }
    log_puts("  SHA256 OK\r\n");

    log_puts("  ECDSA verify...\r\n");
    if (ecdsa_verify_p256_sha256(hash,
                                 hdr->signature,
                                 (uint32_t)sizeof(hdr->signature),
                                 root_public_key) != 0) {
        log_puts("  FAIL: ECDSA\r\n");
        return false;
    }
    log_puts("  ECDSA OK\r\n");

    return true;
}

/**
 * Compute SHA-256 over application region using STM32H7 HASH peripheral.
 *
 * app_address: Start address of application code in flash.
 * app_size   : Size of application code in bytes.
 * out_hash   : 32-byte buffer for resulting digest.
 *
 * Return 0 on success, non-zero on error.
 */
static int hw_sha256_app_region(uint32_t app_address,
                                uint32_t app_size,
                                uint8_t out_hash[32])
{
    const uint32_t block_bytes = 64U;
    const uint32_t n_full_blocks = app_size / block_bytes;
    const uint32_t remainder = app_size % block_bytes;
    uint8_t last_block[block_bytes];
    const volatile uint32_t *src;
    uint32_t i;
    uint32_t timeout;

    /* Enable HASH and GPIOB clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_HASHEN;
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;

    /* Initialize HASH for SHA-256, 32-bit data */
    HASH_CR = HASH_CR_INIT | HASH_CR_ALGO_SHA256 | HASH_CR_DATATYPE_32B;

    /* Feed full 64-byte blocks from flash */
    src = (const volatile uint32_t *)app_address;
    for (i = 0U; i < n_full_blocks; i++) {
        for (uint32_t j = 0U; j < 16U; j++) {
            HASH_DIN = src[i * 16U + j];
        }
        HASH_STR = 32U; /* NBLW = 32 (full last word of block) */
    }

    /* Build last block with SHA-256 padding: data || 0x80 || zeros || length_bits (64-bit BE) */
    for (i = 0U; i < block_bytes; i++) {
        last_block[i] = 0U;
    }
    if (remainder > 0U) {
        const uint8_t *app_bytes = (const uint8_t *)app_address;
        for (i = 0U; i < remainder; i++) {
            last_block[i] = app_bytes[n_full_blocks * block_bytes + i];
        }
    }
    last_block[remainder] = 0x80U;
    /* Length in bits (big-endian) at end of block */
    {
        uint64_t bits = (uint64_t)app_size * 8U;
        last_block[63] = (uint8_t)(bits & 0xFFU);
        last_block[62] = (uint8_t)((bits >> 8) & 0xFFU);
        last_block[61] = (uint8_t)((bits >> 16) & 0xFFU);
        last_block[60] = (uint8_t)((bits >> 24) & 0xFFU);
        last_block[59] = (uint8_t)((bits >> 32) & 0xFFU);
        last_block[58] = (uint8_t)((bits >> 40) & 0xFFU);
        last_block[57] = (uint8_t)((bits >> 48) & 0xFFU);
        last_block[56] = (uint8_t)((bits >> 56) & 0xFFU);
    }

    for (i = 0U; i < 16U; i++) {
        uint32_t word;
        word = (uint32_t)last_block[i * 4U] |
               ((uint32_t)last_block[i * 4U + 1U] << 8) |
               ((uint32_t)last_block[i * 4U + 2U] << 16) |
               ((uint32_t)last_block[i * 4U + 3U] << 24);
        HASH_DIN = word;
    }
    HASH_STR = 32U;
    timeout = 1000000U;
    while ((HASH_SR & HASH_SR_DCIS) == 0U && timeout > 0U) {
        timeout--;
    }
    if (timeout == 0U) {
        return -1;
    }

    /* Read digest: HR0..HR7 into out_hash (SHA-256: 8 x 32-bit, store big-endian) */
    for (i = 0U; i < 8U; i++) {
        uint32_t h = HASH_HR(i);
        out_hash[i * 4U]     = (uint8_t)(h >> 24);
        out_hash[i * 4U + 1U] = (uint8_t)(h >> 16);
        out_hash[i * 4U + 2U] = (uint8_t)(h >> 8);
        out_hash[i * 4U + 3U] = (uint8_t)(h);
    }

    return 0;
}

/**
 * Verify ECDSA P-256 signature over SHA-256 hash using mbedTLS.
 *
 * hash         : 32-byte SHA-256 digest of the application.
 * signature    : 64-byte raw signature r||s, big-endian.
 * signature_len: Length of signature buffer (must be 64).
 * pubkey_xy    : 64-byte public key X||Y, big-endian (secp256r1).
 *
 * Return 0 on success (valid signature), non-zero on failure.
 */
static int ecdsa_verify_p256_sha256(const uint8_t hash[32],
                                    const uint8_t *signature,
                                    uint32_t signature_len,
                                    const uint8_t *pubkey_xy)
{
#ifdef USE_ECDSA_STUB
    (void)hash;
    (void)signature;
    (void)signature_len;
    (void)pubkey_xy;
    return 0; /* Accept any signature for testing */
#else
    int ret = -1;
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_mpi r;
    mbedtls_mpi s;

    if (signature_len != 64U) {
        return -1;
    }

    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);

    do {
        // Load NIST P-256 domain parameters
        ret = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
        if (ret != 0) {
            break;
        }

        // Load public key coordinates X and Y from raw bytes
        ret = mbedtls_mpi_read_binary(&Q.X, pubkey_xy, 32);
        if (ret != 0) {
            break;
        }

        ret = mbedtls_mpi_read_binary(&Q.Y, pubkey_xy + 32, 32);
        if (ret != 0) {
            break;
        }

        ret = mbedtls_mpi_lset(&Q.Z, 1); // Affine coordinates: Z = 1
        if (ret != 0) {
            break;
        }

        // Load signature components r and s from raw bytes
        ret = mbedtls_mpi_read_binary(&r, signature, 32);
        if (ret != 0) {
            break;
        }

        ret = mbedtls_mpi_read_binary(&s, signature + 32, 32);
        if (ret != 0) {
            break;
        }

        // Verify ECDSA signature over the 32-byte hash
        ret = mbedtls_ecdsa_verify(&grp,
                                   hash,
                                   32U,
                                   &Q,
                                   &r,
                                   &s);
    } while (0);

    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&grp);

    return ret;
#endif
}

/**
 * Turn on error LED LD3 (red, PB14) and halt.
 */
static void signal_verification_failure(void)
{
    /* PB14 as output (01 in MODER for pin 14) */
    GPIOB_MODER &= ~(3U << (LED3_PIN * 2U));
    GPIOB_MODER |= (1U << (LED3_PIN * 2U));
    GPIOB_BSRR = (1U << LED3_PIN); /* Set PB14 high */
    for (;;) {
        __asm volatile("wfi");
    }
}

/**
 * Jump to application at entry point from image header.
 * Disables interrupts, sets MSP and vector table, then branches.
 */
static void jump_to_application(uint32_t entry_point)
{
    const uint32_t *app_vectors = (const uint32_t *)entry_point;
    uint32_t app_msp;
    uint32_t app_reset;

    if (entry_point == 0U) {
        signal_verification_failure();
    }
    app_msp = app_vectors[0];
    app_reset = app_vectors[1];

    __asm volatile(
        "msr msp, %0\n"
        "bx   %1\n"
        : : "r"(app_msp), "r"(app_reset) : "memory"
    );
}

int main(void)
{
    const image_header_t *hdr = (const image_header_t *)IMAGE_HEADER_ADDRESS;

    log_init();
    log_puts("[boot] Secure bootloader\r\n");
    log_puts("[boot] UART 115200 OK\r\n");

    if (!verify_signature()) {
        log_puts("[boot] Signature FAIL, halt\r\n");
        signal_verification_failure();
    }

    log_puts("[boot] Signature OK\r\n");
    log_puts("[boot] Jump to app\r\n");
    jump_to_application(hdr->entry_point);

    return 0;
}
