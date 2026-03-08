#!/usr/bin/env python3
"""
Sign application image for Secure Bootloader.
Usage: python scripts/sign_image.py app.bin signed_app.bin
Reads app.bin (raw application binary, linked for 0x08010060), computes SHA-256,
signs with ECDSA P-256 using scripts/root_private_key.pem, prepends image_header_t,
writes signed_app.bin to flash at 0x08010000.
"""
import hashlib
import struct
import sys
import os

# Add script dir for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ecdsa import SigningKey, NIST256p

IMAGE_HEADER_MAGIC = 0x424F4F54  # 'BOOT'
IMAGE_HEADER_ADDRESS = 0x08010000
APP_ENTRY_POINT = 0x08010060  # Vector table after 96-byte header
HEADER_SIZE = 96  # sizeof(image_header_t)


def main():
    if len(sys.argv) != 3:
        print("Usage: sign_image.py <app.bin> <signed_app.bin>")
        sys.exit(1)
    app_path = sys.argv[1]
    out_path = sys.argv[2]
    key_path = os.path.join(os.path.dirname(__file__), "root_private_key.pem")

    with open(app_path, "rb") as f:
        app_data = f.read()
    with open(key_path, "rb") as f:
        sk = SigningKey.from_pem(f.read())

    image_size = len(app_data)
    digest = hashlib.sha256(app_data).digest()
    raw_sig = sk.sign_digest(digest)
    if len(raw_sig) != 64:
        print("Error: expected 64-byte signature, got", len(raw_sig))
        sys.exit(1)

    header = struct.pack(
        "<IIII",
        IMAGE_HEADER_MAGIC,
        1,  # version
        image_size,
        APP_ENTRY_POINT,
    )
    header += b"\x00" * 16  # reserved
    header += raw_sig

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(app_data)
    print(f"Written {len(header) + image_size} bytes to {out_path}")


if __name__ == "__main__":
    main()
