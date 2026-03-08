import os
from ecdsa import SigningKey, NIST256p

def generate_pki():
    # Генерируем секретный ключ на кривой NIST P-256 (стандарт для Embedded)
    private_key = SigningKey.generate(curve=NIST256p)
    public_key = private_key.verifying_key

    # Сохраняем приватный ключ в файл (БЕРЕЧЬ КАК ЗЕНИЦУ ОКА)
    with open("root_private_key.pem", "wb") as f:
        f.write(private_key.to_pem())

    # Сохраняем открытый ключ (его мы положим в код загрузчика)
    with open("root_public_key.pem", "wb") as f:
        f.write(public_key.to_pem())

    # Генерируем C-массив для вставки в код прошивки
    pub_bytes = public_key.to_string()
    print("\n--- СКОПИРУЙ ЭТО В СВОЙ C-КОД (bootloader/inc/keys.h) ---")
    print("const uint8_t root_public_key[] = {")
    print("    " + ", ".join([f"0x{b:02x}" for b in pub_bytes]))
    print("};")
    print("---------------------------------------------------------\n")

if __name__ == "__main__":
    generate_pki()
