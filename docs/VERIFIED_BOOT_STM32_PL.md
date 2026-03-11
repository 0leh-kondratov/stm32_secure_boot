# Zweryfikowany rozruch na STM32: baza wiedzy

Dokument w duchu [3mdeb/verified-boot](https://github.com/3mdeb/verified-boot): jednolita terminologia i kluczowe koncepcje Verified Boot, dostosowane do **STM32** i minimalnego bootloadera z weryfikacją podpisu. Celem nie jest powielanie ogólnej teorii, ale zapewnienie powiązania pomiędzy „koncepcjami → naszego projektu i odniesieniami ST”.

**Odbiorcy:** Twórcy systemów wbudowanych, producenci OEM i wszyscy, którzy chcą bezpiecznie uruchamiać STM32.

**Zastosowanie:** edukacja, projektowanie własnego bezpiecznego rozruchu, porównanie z [X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) i [MCUboot](https://docs.mcuboot.com/).

---

## 1. Łańcuch zaufania na STM32

### 1.1 Root of Trust (RoT) - root zaufania

**RoT** to komponent (sprzęt lub kod), który jest domyślnie zaufany. Zaufanie do niego nie jest sprawdzone: ten, kto go „próbuje”, sam staje się nowym korzeniem. Naruszenie RoT sprawia, że ​​wszystkie kolejne kroki stają się bezsensowne.

Na STM32 rolę RoT pełnią zazwyczaj:

- **Hardware bootloader** (pamięć systemowa, Boot0=1) - zamknięty firmware ST; lub  
- **Pierwszy kod we Flashu** (Boot0=0) to nasz bootloader pod adresem `0x08000000`, jeśli uznamy go za zaufany (napisany w zaufanym środowisku, chroniony przed nadpisaniem przez WRP, jeśli to konieczne).

W tym projekcie **RoT = bootloader na początku Flasha**: nie jest on sprawdzany przez inny kod w krypcie, liczymy na to, że został poprawnie napisany i zabezpieczony przed podstawieniem (opcjonalnie - ochrona przed zapisem).

### 1.2 Root of Trust for Verification (RTV)

**RTV** to komponent, który **sprawdza** kolejny obraz (integralność i autentyczność) **zanim** zostanie na niego przekazana kontrola. Cytat ARM: „niezmienna lokalizacja (taka jak boot ROM), która kryptograficznie weryfikuje pierwszy modyfikowalny kod w systemie przed jego wykonaniem”.

Na STM32:

- **RTV = nasz bootloader**: sprawdza nagłówek obrazu aplikacji (magiczny, rozmiar), skrót (SHA-256) i podpis (ECDSA P-256). Dopiero w przypadku pozytywnej weryfikacji następuje przejście do aplikacji.
- W [X-CUBE-SBSFU](https://www.st.com/en/embedded-software/x-cube-sbsfu.html) taką samą rolę pełni Secure Boot z pakietu; w [MCUboot](https://docs.mcuboot.com/) - bootutil po przeniesieniu na STM32.

### 1.3 Łańcuch zaufania (CoT)

**CoT** to ciąg składników, gdzie każdy kolejny jest **sprawdzany** przez poprzedni. RoT - pierwsze łącze; Gdy tylko kontrola zostanie przeniesiona na niezaufany kod, łańcuch pęka.

W naszym przypadku:

1. **RoT/RTV** - bootloader dla `0x08000000` (ufamy mu).  
2. Sprawdza obraz pod kątem `0x08010000` (nagłówek + podpis).  
3. Jeśli się powiedzie, przekazuje kontrolę do `punktu wejścia` (np. `0x08010080`) - **drugie łącze CoT**.  
4. Dalsze aktualizacje (jeśli są dostępne) mogą kontynuować łańcuch: aplikacja sprawdza następny obraz itp.

---

## 2. Rozruch zweryfikowany i zmierzony (krótko)

- **Verified Boot** — weryfikacja obrazu **przed** uruchomieniem (podpis/hash). Nieprzetestowany kod **nie wykonuje się**. Nasz bootloader implementuje dokładnie to.
- **Measured Boot** - **napraw** to, co zostało uruchomione (na przykład rejestrowanie skrótów w TPM lub Flash), bez decydowania, czy to uruchomić, czy nie. Do późniejszego audytu i certyfikacji.

Na STM32 bez modułu TPM można imitować zmierzony rozruch (na przykład zapisanie skrótu obrazu w oddzielnym obszarze Flash lub NVRAM), ale typowym scenariuszem dla systemów wbudowanych jest **rozruch zweryfikowany** (blokowanie niepodpisanego/fałszywego oprogramowania). Bardziej szczegółowe porównanie można znaleźć w [3mdeb/verified-boot](https://github.com/3mdeb/verified-boot/blob/master/verified_boot_main.md).

---

## 3. Integralność i autentyczność

- **Integralność:** Dane nie zostały zmienione bez pozwolenia. Dostarczone przez **hash** (używamy SHA-256 w treści aplikacji). Zmiana nawet bajtu powoduje zmianę skrótu.
- **Autentyczność:** zaufanie do źródła danych. Opatrzone **podpisem** (posiadamy ECDSA P-256 poprzez skrót). Tylko właściciel klucza prywatnego mógł utworzyć podpis; klucz publiczny w bootloaderze sprawdza to.

Sam hash nie gwarantuje źródła (wróg może zastąpić zarówno obraz, jak i hash). Sam podpis, bez odniesienia do skrótu, nie jest powiązany z treścią. Dlatego w naszym formacie: **najpierw obliczyliśmy hash obrazu, następnie sprawdziliśmy podpis z tego hasha** – otrzymujemy zarówno integralność, jak i autentyczność (oraz niezaprzeczalność).

Format nagłówka w projekcie: [common/image_header.h](../common/image_header.h) - magia, wersja, rozmiar_obrazu, punkt_wejściowy, zarezerwowany, podpis[64].

---

## 4. Na jakim etapie tego schematu się znajdujemy

| Element | W tym projekcie (stm32_secure_boot) |
|------------|----------------------|
| RoT | Bootloader o 0x08000000 (pierwszy kod z Flasha o Boot0=0) |
| RTV | Ten sam program ładujący: sprawdzenie nagłówka, SHA-256, ECDSA przed skokiem |
| CoT | Bootloader → (zaznacz) → Aplikacja |
| Zweryfikowany rozruch | Tak: niepodpisany/nieprawidłowy obraz nie zostanie uruchomiony |
| Zmierzony but | Nie (jeśli chcesz, możesz dodać wpis skrótu we Flashu) |
| Uczciwość | SHA-256 nad korpusem aplikacji |
| Autentyczność | ECDSA P-256, klucz publiczny w bootloaderze (keys.h) |

Montaż i oprogramowanie: [docs/BUILD_PL.md](BUILD_PL.md). Oficjalne i podobne rozwiązania: [docs/REFERENCES_PL.md](REFERENCES_PL.md).

---

## 5. Minimalne wymagania dla „prawidłowego” zweryfikowanego rozruchu na STM32

(Pomysły wspólne dla naszego podejścia to X-CUBE-SBSFU i MCUboot.)

- **RoT/RTV na obszarze chronionym**  
  Kodu bootloadera nie należy zmieniać. W praktyce: nagrywanie tylko na zaufanym kanale; jeśli to konieczne, włącz ochronę zapisu (WRP) w sektorach z programem ładującym (patrz Instrukcja obsługi dla konkretnej serii).

- **Sprawdź przed wykonaniem**  
  Nie wykonuj ani jednego bajtu kodu aplikacji, dopóki podpis (oraz, jeśli to konieczne, wersja, rozmiar) nie zostanie pomyślnie zweryfikowany.

- **Minimalna kwota TCB**  
  Zaufana baza obliczeniowa obejmuje tylko: program ładujący, bibliotekę kryptograficzną (lub jednostkę sprzętową), magazyn kluczy publicznych. Im mniej kodu w bazie TCB, tym łatwiejszy audyt i mniejsza powierzchnia ataku.

- **Klucze**  
  Klucz prywatny – tylko po stronie kompilacji/CI; Urządzenie zawiera tylko klucz publiczny (lub skrót klucza publicznego). Nie przechowuj klucza prywatnego w programie Flash MK.

- **Powtarzalność**  
  Obraz aplikacji musi być podpisany deterministycznie (to samo wejście → ten sam podpis), aby można było zweryfikować kompilację.

---

## 6. Zagrożenia i ograniczenia (w skrócie)

- **Zastąpienie bootloadera:** bez WRP atakujący z dostępem do programowania Flash może zapisać swój kod pod adresem 0x08000000. Środek: WRP, kontrola dostępu do programatora, zabezpieczenie firmware w produkcji.
- **Wyciek klucza prywatnego:** Każdy może podpisywać obrazy. Rozwiązanie: klucz znajduje się wyłącznie na bezpiecznym serwerze kompilacji, a nie w oprogramowaniu sprzętowym lub w repozytorium w postaci zwykłego tekstu.
- **Przywracanie wersji (obniżanie wersji):** Obecny minimalny program ładujący nie sprawdza dokładnie wersji (w nagłówku znajduje się pole wersji, ale zasada „nie uruchamiaj starej wersji” nie jest zaimplementowana). Jeśli to konieczne, dodaj kontrolę wersji względem minimalnej akceptowalnej wersji zapisanej w programie ładującym lub w bajtach opcji.

Ogólne szczegółowe modele zagrożeń dla Verified Boot można znaleźć w [3mdeb/verified-boot (threat-model)](https://github.com/3mdeb/verified-boot/blob/master/threat-model.md).

---

## 7. Link do instancji urządzenia (MB/karta)

**Powiązanie z instancją** - ograniczenie uruchamiania oprogramowania tylko na konkretnej płycie/mikrokontrolerze (poprzez unikalny identyfikator lub klucz). Jest to konieczne, aby obraz pobrany z jednej płyty nie mógł zostać uruchomiony na innej, lub aby licencjonować oprogramowanie „na jedną kartę”.

### 7.1 Unikalny identyfikator mikrokontrolera (UID)

STM32 ma **96-bitowy unikalny identyfikator urządzenia** wbudowany w chip (tylko do odczytu, nie można go zmienić). Adres w RM0433 dla STM32H743/753/750:

- **Podstawa UID:** `0x1FF1E800`
- **3 słowa po 32 bity:** UID[0], UID[1], UID[2] - łącznie 96 bitów.

Odczyt kodu bootloadera (bez HAL):

```c
#define UID_BASE  0x1FF1E800UL
#define UID0      (*(const volatile uint32_t *)(UID_BASE + 0x00))
#define UID1      (*(const volatile uint32_t *)(UID_BASE + 0x04))
#define UID2      (*(const volatile uint32_t *)(UID_BASE + 0x08))
```

Dodatkowo STM32H7 ma **Device ID** (identyfikator typu chipa) pod adresem `0x1FF1E7E0` (2 bajty). UID rozróżnia instancje tego samego typu.

### 7.2 Metody łączenia

| Podejście | Esencja | Plusy | Wady |
|-------|------|--------|------------|
| **UID w podpisanych danych** | Podczas podpisywania obrazu blok „podpisany” zawiera elementy magiczne, rozmiar, wersję i **hasz UID** (lub sam UID). Podpis = Znak(SHA256(obrazek \|\| UID_hash)). Program ładujący odczytuje UID, oblicza UID_hash, zbiera ten sam blok i weryfikuje podpis. | Sztywne wiązanie do jednej deski. | Każda płytka ma swój własny obraz (lub trzeba go podpisać na urządzeniu/w produkcji zgodnie z listą UID). |
| **Opcjonalne pole nagłówka** | W `reserved[]` lub nowym polu w `image_header_t`: na przykład `uint8_t uid_hash[32]` (SHA-256 z UID). Jeśli pole ma wartość zero, nie sprawdzamy UID (tak jak to robimy teraz). Jeśli nie wynosi zero, program ładujący porównuje SHA256(UID) z tym polem. | Jeden obraz może być rozesłany każdemu, a powiązanie można włączyć tylko tam, gdzie jest to konieczne (podpisz obraz zamiennikiem UID_hash dla konkretnej płytki). | Konieczne jest wygenerowanie obrazu na miejscu produkcji/u klienta dla konkretnego UID lub przechowywanie tabeli UID →obraz. |
| **Klucz unikalny dla urządzenia** | Każda płytka ma swój własny klucz (w OTP, eFuse lub zewnętrznym elemencie zabezpieczającym, np. STSAFE-A110). Obraz jest szyfrowany lub podpisywany pod tym kluczem. Bootloader weryfikuje podpis za pomocą klucza tego urządzenia. | Maksymalne wiązanie, ochrona przed kopiowaniem obrazu. | Bardziej złożona infrastruktura: generowanie/wstrzykiwanie kluczy w produkcji, ewentualnie oddzielny chip. |
| **UID jako „sól” podczas sprawdzania** | Podpis pozostaje sam (bez UID), ale przed sprawdzeniem podpisu bootloader dodaje do logiki UID (na przykład oblicza hash(image \|\| UID) i sprawdza podpis z tego hasha). Wtedy obraz podpisany bez uwzględnienia UID nie będzie działał na innym urządzeniu. | Nie ma potrzeby przechowywania listy UID na serwerze. | Klucz prywatny musi być używany w środowisku, w którym znany jest UID (oprogramowanie produkcyjne) lub schemat staje się bardziej złożony (na przykład usługa produkcyjna podpisuje się na żądanie za pomocą UID). |

W **bieżącym projekcie** nie ma powiązania z UID: ten sam plik `signed_app.bin` można wgrać na dowolną płytkę z tym samym typem MCU. Dodanie kotwicy jest rozszerzeniem formatu nagłówka i logiki podpisu/walidacji (patrz poniżej).

### 7.3 Gdzie wpisać UID w naszym formacie

- **Opcja A:** użyj pola **reserved[16]** w [image_header.h](../common/image_header.h): na przykład pierwsze 12 bajtów = UID (96 bitów) lub 32 bajty = SHA-256(UID). Po sprawdzeniu podpisu (tak jak teraz) bootloader dodatkowo sprawdza: jeśli zarezerwowane nie jest równe zero, odczytuje UID i porównuje go z zarezerwowanym (lub porównuje SHA256(UID) z zarezerwowanym).
- **Opcja B:** rozwiń nagłówek: dodaj pole `uint8_t uid_binding[32]` (SHA-256 z UID urządzenia docelowego). Budując obraz dla konkretnej płytki, skrypt podpisu żąda UID (lub odczytuje z pliku), oblicza SHA256(UID), zapisuje do uid_binding i podpisuje cały nagłówek wraz z treścią. Program ładujący odczytuje UID, oblicza SHA256(UID), porównuje z uid_binding; w przypadku niezgodności odmawia uruchomienia.

W obu opcjach **podpis musi obejmować** pole z UID/uid_binding, w przeciwnym razie możesz podmienić UID w nagłówku bez zmiany podpisu.

### 7.4 Oficjalne decyzje ST

- **X-CUBE-SBSFU:** obsługa zewnętrznego elementu zabezpieczającego (STSAFE-A110) do przechowywania kluczy i w razie potrzeby powiązania z urządzeniem.
- **SFI (Secure Firmware Install):** w obsługiwanych seriach (H5, H7RS, L5, U5, itp.) - bezpieczna instalacja obrazu z powiązaniem z urządzeniem na etapie produkcji (klucze/sekrety w HSM, obraz jest instalowany tylko w „swoim” urządzeniu).
- **Bajty opcji / OTP:** w części STM32 znajdują się obszary OTP lub bajty opcji do przechowywania danych specyficznych dla urządzenia; można w nich przechowywać skrót UID lub klucz (w zależności od serii i polityki bezpieczeństwa).

Konkluzja: powiązanie z MB/kartą jest realizowane poprzez **UID** (odczytywane przez `0x1FF1E800`) i/lub **unikalny klucz na urządzeniu**; Nasz minimalny bootloader jeszcze go nie ma, ale dodaje go rozszerzenie nagłówka i skryptu podpisu zgodnie z powyższymi opcjami.

---

## 8. Linki

- [3mdeb/verified-boot](https://github.com/3mdeb/verified-boot) - baza wiedzy na temat Verified Boot (koncepcje RoT, RTV, zweryfikowane vs zmierzone).
- [REFERENCES_PL.md](REFERENCES_PL.md) - oficjalne rozwiązania od ST (X-CUBE-SBSFU, Trusted Package Creator) i MCUboot.
- [BUILD_PL.md](BUILD_PL.md) - montaż i firmware tego projektu.
- NIST SP 800-193 (PIC), NIST SP 800-155 (integralność BIOS-u), TCG Glosariusz - podstawowe definicje RoT/RTM/RTV (cytowane w 3mdeb).

Jeśli później dodamy pomiar (measured boot) lub politykę wersji, dokument ten można rozszerzyć o odpowiednie sekcje bez zmiany opisanego już schematu RoT/RTV/CoT.