import os
from ecdsa import SigningKey, NIST256p

def generate_pki():
# Wygeneruj tajny klucz za pomocą krzywej NIST P-256 (standard dla Embedded)
    private_key = SigningKey.generate(curve=NIST256p)
    public_key = private_key.verifying_key

# Zapisz klucz prywatny do pliku (Trzymaj go jak źrenicę oka)
    with open("root_private_key.pem", "wb") as f:
        f.write(private_key.to_pem())

# Zapisz klucz publiczny (umieścimy go w kodzie bootloadera)
    with open("root_public_key.pem", "wb") as f:
        f.write(public_key.to_pem())

# Wygeneruj tablicę C do wstawienia do kodu oprogramowania sprzętowego
    pub_bytes = public_key.to_string()
print("\n--- KOPIUJ TO DO SWOJEGO KODU C (bootloader/inc/keys.h) ---")
    print("const uint8_t root_public_key[] = {")
    print("    " + ", ".join([f"0x{b:02x}" for b in pub_bytes]))
    print("};")
    print("---------------------------------------------------------\n")

if __name__ == "__main__":
    generate_pki()
