# Co jest potrzebne do utworzenia i flashowania obrazu dla STM32

W skrócie: co zainstalować i jakie kroki wykonać, aby **zbudować** i **flashować** obraz (łącznie z weryfikacją podpisu) - w **naszym projekcie** i przy korzystaniu z **oficjalnych rozwiązań ST**.

---

## 1. W tym projekcie (stm32_secure_boot)

Minimalny zestaw do montażu podpisanego obrazu i oprogramowania sprzętowego w NUCLEO-H743ZI2.

### 1.1 Co musisz zainstalować

| Składnik | Miejsce docelowe | Skąd zdobyć / jak zainstalować |
|-----------|------------|----------------------------|
| **ARM GCC** | Kompilacja bootloadera i aplikacji | Linux: `sudo apt install gcc-arm-none-eabi`. Windows: [Wbudowany zestaw narzędzi GNU Arm](https://developer.arm.com/downloads/-/gnu-rm). macOS: `brew install arm-none-eabi-gcc`. |
| **Pyton 3** | Podpisywanie obrazu (ECDSA) | Systemowy Python lub venv projektu `scripts/venv`. Projekt ma już `ecdsa` w venv. |
| **pierwszy błysk** | Nagrywanie obrazu do pamięci Flash przez USB | Linux: `sudo apt install stlink-tools`. Lub skompiluj z [stlink-org/stlink](https://github.com/stlink-org/stlink). |
| **Opłaty** | Urządzenie docelowe | NUCLEO-H743ZI2 (MB1364) ze złączem USB (wbudowany ST-Link). |

Dodatkowo: **minicom** lub **putty** do podglądu UART (115200 8N1), jeśli potrzebujesz logu.

### 1.2 Tworzenie i flashowanie obrazu

**Budowanie obrazu (bootloader + podpisana aplikacja):**

```bash
cd /path/to/stm32_secure_boot
make bootloader
make signed-app
```

Otrzymujesz: `build/bootloader/bootloader.bin`, `build/app/app.bin`, `build/app/signed_app.bin`.

**Firware z jednym skryptem (zalecane):**

```bash
./scripts/flash.sh
```

Sam skrypt składa bootloader, aplikację, podpisuje i zapisuje połączony obraz z 0x08000000.

**Lub ręcznie:** połącz bootloader (64 KB) + Sign_app.bin i flash (patrz [BUILD_PL.md](BUILD_PL.md)).

Szczegółowa instrukcja krok po kroku: [BUILD_PL.md](BUILD_PL.md).

---

## 2. Oficjalne decyzje ST

Aby utworzyć i flashować obraz w **X-CUBE-SBSFU** lub w **Trusted Package Creator**, potrzebne są inne narzędzia i procedury.

### 2.1 X-CUBE-SBSFU (Secure Boot + Secure Firmware Update)

**Co to jest:** Pakiet rozszerzeń STM32Cube z gotowym bootloaderem, weryfikacją podpisu (RSA/ECDSA) i aktualizacją poprzez UART (Ymodem). Obsługiwany jest STM32H7 (w tym H743).

**Co potrzebujesz:**

| Składnik | Miejsce docelowe |
|-----------|------------|
| **X-CUBE-SBSFU** | Sam pakiet ze źródłami i przykładami. |
| **STM32CubeMX** (opcjonalnie) | Generowanie kodu dla Twojej tablicy i konfiguracja SBSFU. |
| **Środowisko kompilacji** | STM32CubeIDE (zalecany ST) lub Makefile/IAR/Keil - korzystając z przykładów z pakietu. |
| **Dokumentacja** | Podręcznik użytkownika **DM00414687** (Pierwsze kroki), Nota aplikacyjna **DM00414677** (Przewodnik integracji). |

**Kroki (ogólny zarys):**

1. Pobierz [X-CUBE-SBSFU] (https://www.st.com/en/embedded-software/x-cube-sbsfu.html) ze st.com (wymagane konto ST).
2. Rozpakuj paczkę, otwórz przykład dla swojej serii (np. STM32H7) w STM32CubeIDE lub zmontuj według instrukcji z DM00414687.
3. Wygeneruj klucze i podpisz obraz aplikacji zgodnie z procedurą z pakietu (skrypty/narzędzia znajdują się w zestawie).
4. Flash: najpierw bootloader SBSFU, potem aplikacja – sposób zależy od przykładu (STM32CubeProgrammer, bootloader UART itp.).

Zobacz **DM00414687** i **DM00414677**, aby zapoznać się z konkretnymi poleceniami i elementami menu.

### 2.2 STM32 Trusted Package Creator (podpisywanie i pakowanie obrazów)

**Co to jest:** narzędzie do podpisywania i pakowania obrazów (w tym do bezpiecznej instalacji oprogramowania sprzętowego). W zestawie **STM32CubeProgrammer**.

**Co potrzebujesz:**

| Składnik | Miejsce docelowe |
|-----------|------------|
| **Programista STM32Cube** | Obejmuje programistę GUI i **Zaufany twórca pakietów**. |
| **Dokumentacja** | Instrukcja obsługi **UM2238** (Twórca zaufanych pakietów STM32). |

**Gdzie to zdobyć:** [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) (st.com, sekcja Narzędzia programistyczne).

**Kroki (ogólny zarys):**

1. Zainstaluj STM32CubeProgrammer (Linux/Windows/macOS).
2. Uruchom **Trusted Package Creator** (z tego samego pakietu lub z menu programatora - patrz UM2238).
3. Prześlij obraz aplikacji (.bin/.hex), ustaw klucze i parametry podpisywania/szyfrowania za pomocą UM2238.
4. Uzyskaj podpisany/spakowany obraz i wgraj go za pomocą STM32CubeProgrammer lub bootloadera (na przykład SBSFU), w zależności od wybranego scenariusza.

Format obrazu i opcje (RSA, ECDSA, AES-GCM itp.) opisano w **UM2238**.

### 2.3 SBSFU firmy MCUboot (L5, U5, H5, H7RS - nie dla klasycznego H743)

Dla **STM32H743** oficjalna ścieżka z gotowymi przykładami to **X-CUBE-SBSFU**. Rozwiązania bazujące na MCUboot (OEMiRoT/OEMuRoT) w Cube są dostępne dla innych serii (L5, U5, WBA5, H5, H7RS), a nie dla zwykłego H743.

### 2.4 Uwagi dotyczące aplikacji (ogólne zrozumienie)

- **AN5447** - recenzja Bezpiecznego rozruchu i Bezpiecznej aktualizacji oprogramowania sprzętowego (TrustZone itp.).
- **AN4992** - Wprowadzenie do bezpiecznej instalacji oprogramowania sprzętowego (SFI).

Przydatne w architekturze i terminologii; bezpośrednie instrukcje krok po kroku dotyczące „jak flashować” znajdują się w pakietach instrukcji obsługi (DM00414687, UM2238).

---

## 3. Tabela przestawna

| Akcja | Ten projekt | X-CUBE-SBSFU | Zaufany twórca pakietów |
|----------|-------------|--------------|--------------------------|
| **Budowanie wizerunku** | `utwórz program ładujący` + `utwórz aplikację` | STM32CubeIDE / Przykłady tworzenia pakietów | Nie zbiera, tylko podpisuje gotowy .bin |
| **Podpis** | `scripts/sign_image.py` (Python + ecdsa) | Narzędzia/skrypty z pakietu SBSFU | Zaufany twórca pakietów (GUI) |
| **Oprogramowanie sprzętowe** | `st-flash` lub `./scripts/flash.sh` | STM32CubeProgrammer lub wbudowany bootloader poprzez UART | STM32CubeProgrammer (lub po podpisaniu - poprzez SBSFU) |
| **Dokumentacja** | [BUILD_PL.md](BUILD_PL.md) | DM00414687, DM00414677 | UM2238 |

---

## 4. Podsumowanie

- **Aby utworzyć i flashować obraz w naszym projekcie:** zainstaluj ARM GCC, Python (i zależności podpisywania), st-flash; Płyta NUCLEO-H743ZI2 przez USB. Następnie: `./scripts/flash.sh` lub kroki z [BUILD_PL.md](BUILD_PL.md).
- **Aby skorzystać z oficjalnych rozwiązań ST:** pobierz **X-CUBE-SBSFU** i/lub **STM32CubeProgrammer** (z zaufanym kreatorem pakietów), otwórz **DM00414687** i **UM2238** i postępuj zgodnie z odpowiednimi sekcjami dotyczącymi Twojej planszy i scenariusza.

**Szybki start krok po kroku na X-CUBE-SBSFU (pobierz → rozpakuj → znajdź projekt H7 → zmontuj i flashuj):** zobacz [SBSFU_QUICKSTART_PL.md](SBSFU_QUICKSTART_PL.md).

Jeśli potrzebne są bezpośrednie linki do plików PDF (DM00414687, DM00414677, UM2238, AN5447, AN4992), można je znaleźć na [st.com](https://www.st.com) w sekcji dokumentacji [X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) i [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html).
