# Oficjalne i podobne rozwiązania: Bezpieczny rozruch dla STM32

Krótki przegląd referencji i oficjalnych przykładów ze ST i społeczności - dla porównania z obecnym minimalnym bootloaderem (STM32H743, ECDSA P-256 + SHA-256).

**Terminologia i koncepcje Verified Boot w odniesieniu do STM32:** patrz [VERIFIED_BOOT_STM32_PL.md](VERIFIED_BOOT_STM32_PL.md).

**Co jest potrzebne do stworzenia i flashowania obrazu (nasz projekt i oficjalny ST):** zobacz [WHAT_NEEDED_STM32_PL.md](WHAT_NEEDED_STM32_PL.md).

---

## 1. Oficjalne decyzje ST

### 1.1 X-CUBE-SBSFU (Secure Boot and Secure Firmware Update)

- **Co to jest:** Pakiet rozszerzeń STM32Cube - pełnoprawny bezpieczny rozruch + bezpieczna aktualizacja oprogramowania sprzętowego.
- **Gdzie:** [ST X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) (pobierz ze st.com).
- **Obsługiwane serie:** w tym **STM32H7** (w tym STM32H743), G0, G4, F4, F7, L0, L1, L4, L4+, WB, WL.
- **Możliwości:**
- sprawdzanie autentyczności i integralności aplikacji przed uruchomieniem;
- aktualizacja poprzez UART (Ymodem) itp.;
- kryptografia asymetryczna (RSA, ECDSA) lub symetryczna;
- opcjonalne szyfrowanie obrazu, integracja z STSAFE-A110.
- **Dokumentacja:**
  - User Manual: DM00414687 (Getting started with X-CUBE-SBSFU);
  - Application Note: DM00414677 (Integration guide);
  - Technical Note: TN1387 (Security evaluation).

**Zalety:** Rozwiązanie referencyjne ST, wiele płyt. **Wady:** ciężki ze względu na strukturę projektu i pamięć.

---

### 1.2 SBSFU firmy MCUboot (nowy kierunek ST)

- **Co to jest:** implementacja bezpiecznego rozruchu w oparciu o otwarty **MCUboot**.
- **ST Wiki:** [SBSFU autorstwa MCUboot](https://wiki.st.com/stm32mcu/wiki/Security:SBSFU_by_MCUboot).
- **Gdzie są przykłady:** w pakietach STM32Cube dla **STM32L5, STM32U5, STM32WBA5** (TF-M), a także **STM32U0, STM32H5, STM32H7RS** (OEMiRoT / OEMuRoT).
- **Dla STM32H7 „classic” (H743 itp.):** głównym oficjalnym przykładem bezpiecznego rozruchu jest nadal **X-CUBE-SBSFU** (punkt 1.1). OEMiRoT/OEMuRoT skupiają się na H7RS (H7R/H7S z TrustZone).

---

### 1.3 STM32 Trusted Package Creator

- **Co to jest:** narzędzie z zestawu **STM32CubeProgrammer** do podpisywania i pakowania obrazów.
- **Gdzie:** dotyczy [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).
- **Funkcje:** podpisywanie obrazu, bezpieczna instalacja oprogramowania sprzętowego (SFI), szyfrowanie (AES-GCM), przygotowanie pakietów do produkcji.
- **Dokumentacja:** Podręcznik użytkownika UM2238 (Twórca zaufanych pakietów STM32).

Może być używany jako alternatywa dla skryptu podpisu (np. `scripts/sign_image.py`) w celu zapewnienia zgodności z narzędziami ST.

---

### 1.4 Notatki aplikacyjne Bezpieczeństwo ST

- **AN5447** - recenzja Bezpiecznego rozruchu i Bezpiecznej aktualizacji oprogramowania sprzętowego na Arm TrustZone dla STM32.
- **AN4992** - Wprowadzenie do bezpiecznej instalacji oprogramowania sprzętowego (SFI) dla STM32.

Ma sens dla ogólnego zrozumienia architektury ST i terminologii.

---

## 2. Otwórz referencje

### 2.1 3mdeb/verified-boot (pojęcia i terminologia)

- **Repozytorium:** [3mdeb/verified-boot](https://github.com/3mdeb/verified-boot) (rozwidlenie adrelanos/verified-boot).
- **Co to jest:** baza wiedzy na temat **Verified Boot** - dokumentacja i analiza mechanizmów weryfikacji bootów w branży. Skoncentruj się na wiarygodnych źródłach (NIST, TCG) i powszechnej terminologii.
- **Zawartość:**
- **Trust, Root of Trust (RoT), Chain of Trust (CoT), TCB** – definicje.
- **RTV (Root of Trust for Verification)** - „niezmienny kod (na przykład boot ROM), który kryptograficznie weryfikuje pierwszy zmodyfikowany kod przed jego wykonaniem” (cytat z ARM BBSR). Obecny bootloader w projekcie to zasadniczo minimalny RTV.
- **Rozruch zweryfikowany a rozruch zmierzony** - zweryfikowany sprawdza podpis/obraz *przed* uruchomieniem i może blokować niezweryfikowany kod; zmierzył tylko *zapisuje* to, co zostało uruchomione (do późniejszej analizy i certyfikacji).
- **Integralność i autentyczność** — hash zapewnia integralność; podpis (ECDSA/RSA) zapewnia autentyczność i niezaprzeczalność.
- **Zasięg repozytorium:** głównie PC/UEFI i Linux (UEFI Secure Boot, Intel Boot Guard, dm-verity, LUKS, TPM). Celem jest edukacja i możliwie otwarty standard Verified Boot for Linux (Kicksecure).
- **Dla STM32:** nie ma bezpośrednich przykładów mikrokontrolerów, ale definicje RoT, RTV, zweryfikowane vs zmierzone oraz „integralność + autentyczność” mają zastosowanie do każdego bezpiecznego rozruchu, w tym minimalnego modułu ładującego z ECDSA.

QUERY LENGTH LIMIT EXCEEDED. MAX ALLOWED QUERY : 500 CHARS

### 2.2 MCUboot (upstream)

- **Witryna:** [docs.mcuboot.com](https://docs.mcuboot.com/).
- **Repozytorium:** [mcu-tools/mcuboot](https://github.com/mcu-tools/mcuboot).
- **Co daje:** ujednolicony format obrazu (nagłówek, TLV, podpis), obsługa ECDSA P-256, RSA, Ed25519, SHA-256/384/512, szyfrowanie obrazu.
- **Port ST dla STM32:** [STMicroelectronics/stm32-mw-mcuboot](https://github.com/STMicroelectronics/stm32-mw-mcuboot) - możesz zobaczyć strukturę projektu i integrację z STM32.

Obecny projekt jest teoretycznie bliższy „minimalnemu bootloaderowi + własnemu formatowi nagłówka”, ale format i logika MCUboot mogą być użyte jako odniesienie (nagłówek, TLV, kolejność sprawdzania).

---

### 2.3 STM32H7RS / OEMiRoT (dla nowego H7 z TrustZone)

- **Wiki:** [OEMiRoT dla STM32H7R](https://wiki.st.com/stm32mcu/wiki/Security:OEMiRoT_for_STM32H7R), [Funkcje zabezpieczeń w STM32H7RS](https://wiki.st.com/stm32mcu/wiki/Security:Security_features_on_STM32H7RS_MCUs).
- **Dotyczy:** STM32H7R/H7S (H7RS), nie dotyczy zwykłego H743. Na H743 oficjalna ścieżka to X-CUBE-SBSFU lub Twój własny minimalny program ładujący, jak w tym repozytorium.

---

## 3. Porównanie z bieżącym projektem

| Aspekt | Bieżący projekt (stm32_secure_boot) | X-CUBE-SBSFU | Port MCUboot/ST |
|---------------------|-----------------------------------|-------------------|---------------------|
| Platforma | STM32H743ZI (NUCLEO-144) | Wiele odcinków, m.in. H7 | Różne MCU, porty dla STM32 |
| Podpis | ECDSA P-256 + SHA-256 (nadal odcinek) | RSA/ECDSA/symetryczny | ECDSA, RSA, Ed25519 |
| Aktualizacja bezprzewodowa | Nie | Tak (Ymodem itp.) | Tak (różne transporty) |
| Trudność | Minimalne | Wysoki | Średnia |
| Format obrazu | Własny (image_header_t + aplikacja) | Twój | Standardowy MCUboot |

---

## 4. Co wziąć za podstawę rozwoju

- **Pozostań minimalny:** zmodyfikuj bieżący bootloader (prawdziwy ECDSA, ochrona klucza, jeśli to konieczne - format zgodny z TLV jak w MCUboot).
- **Potrzebujesz „jak ST”:** pobierz **X-CUBE-SBSFU**, zmontuj przykład dla STM32H7 i prześlij pomysły (znaczniki flash, sprawdź kolejność, format nagłówka, jeśli to konieczne).
- **Potrzebujesz standardowego formatu i aktualizacji:** spójrz na **STMicroelectronics/stm32-mw-mcuboot** i albo przenieś MCUboot na H743, albo przenieś obraz do formatu MCUboot i użyj ich bootutil tylko do testów.

Jeśli potrzebujesz bezpośrednich linków do plików PDF (AN5447, UM2238 itp.), można je znaleźć według nazwy na [st.com](https://www.st.com) w sekcji Dokumentacja odpowiedniego produktu.
