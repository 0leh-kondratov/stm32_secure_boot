# Szybki start: X-CUBE-SBSFU na STM32

Krok po kroku: pobierz paczkę, znajdź projekt dla STM32H7/NUCLEO i uruchom go na płytce.

---

## 1. Pobierz X-CUBE-SBSFU

### 1.1 Gdzie pobrać

- **Strona pakietu:** [st.com/en/embedded-software/x-cube-sbsfu.html](https://www.st.com/en/embedded-software/x-cube-sbsfu.html)
- Na stronie kliknij **「Pobierz oprogramowanie」** lub **「Pobierz」** (lub „Zasoby oprogramowania” → najnowsza wersja).
- Do pobrania wymagane jest **konto ST**. Jeśli go tam nie ma, zarejestruj się na [st.com](https://www.st.com).

### 1.2 Co zostanie pobrane

- Plik taki jak `en.x-cube-sbsfu-vX.X.X.zip` (lub `x-cube-sbsfu.zip`). Wersja w nazwie może się różnić.
- Po rozpakowaniu otrzymasz katalog `STM32CubeExpansion_Secure_Boot_SBSFU_VX.X.X` (lub podobny) z podfolderami:
- **Projekty** - przykłady dla różnych płytek i serii (m.in. STM32H7);
- **Oprogramowanie pośrednie** – Secure Engine, biblioteka kryptograficzna itp.;
  - **Documents** — PDF (User Manual, Integration guide).

### 1.3 Gdzie to umieścić

Polecane obok naszego projektu, aby nie pomylić z naszym kodem:

```text
/data/projects/
├── stm32_secure_boot/ # nasz projekt
├── STM32CubeH7/ # już dostępny (oficjalna kostka)
└── STM32CubeExpansion_Secure_Boot_SBSFU_Vx.x.x/ # rozpakowany X-CUBE-SBSFU
```

Lub w dowolne dogodne miejsce; następnie wszędzie podstaw swoją ścieżkę do rozpakowanego pakietu.

---

## 2. Znajdź projekt dla STM32H7/NUCLEO

Po rozpakowaniu przejdź do katalogu **Projekty**:

```bash
cd /path/to/STM32CubeExpansion_Secure_Boot_SBSFU_Vx.x.x/Projects
ls
```

Zwykle w środku znajdują się podfoldery według serii i plansz, na przykład:

- `STM32H743ZI-Nucleo144/` lub `NUCLEO-H743ZI/`
- lub folder współdzielony `STM32H7xx/` z podfolderami na tablice.

Przejdź do folderu odpowiadającego **NUCLEO-H743ZI** (lub Twojej płycie). Wewnątrz może znajdować się:

- **SBSFU_Boot/** — obraz bootloadera (Secure Boot);
- **SBSFU_App/** - przykładowa aplikacja;
- pliki projektu **.ioc** (STM32CubeMX), **.project** (STM32CubeIDE).

Na pierwsze uruchomienie wystarczy znaleźć **gotowy projekt dla swojej płytki** (NUCLEO-H743ZI).

---

## 3. Złóż i flashuj

###Opcja A: STM32CubeIDE (zalecany ST)

1. Zainstaluj [STM32CubeIDE] (https://www.st.com/en/development-tools/stm32cubeide.html) ze strony st.com.
2. **Plik → Otwórz projekty z systemu plików** (lub Importuj → Istniejące projekty) i określ katalog projektu, na przykład:
   `.../Projects/STM32H743ZI-Nucleo144/SBSFU_Boot`.
3. Zbuduj projekt (Projekt → Zbuduj projekt).
4. Podłącz NUCLEO przez USB, wybierz **Uruchom** lub **Debuguj** – firmware zostanie sflashowany i uruchomiony z IDE.
5. Podobnie otwórz i zbuduj **SBSFU_App**, następnie flashuj aplikację zgodnie z instrukcjami z pakietu User Manual (często przez Ymodem przez UART lub osobny krok w IDE).

Kolejność „najpierw uruchomienie, potem aplikacja” i sposób flashowania aplikacji opisano w **Instrukcji obsługi (DM00414687 / UM2262)** dołączonej do pakietu.

### Opcja B: Makefile (jeśli przykład zawiera Makefile)

Niektóre przykłady SBSFU mają plik Makefile:

```bash
cd /path/to/Projects/STM32H743ZI-Nucleo144/SBSFU_Boot
make
```

Po zakończeniu kompilacji w katalogu pojawią się pliki `.bin` lub `.elf`. Możesz flashować w ten sposób:

```bash
st-flash write build/sbsfu_boot.bin 0x08000000
```

(Nazwę pliku i ścieżkę `build/` znajdziesz w aktualnej wersji pakietu.)

Następnie, korzystając z dokumentacji pakietu, wykonaj flashowanie aplikacji (często pod inny adres, np. slot aplikacji).

### Opcja C: Dokumentacja pakietu

Dokładne kroki dotyczące Twojej płyty i konfiguracji (pojedyncze/dwa gniazda, skąd można sflashować aplikację) zobacz:

- **Instrukcja obsługi** - w folderze `Documents` rozpakowanego X-CUBE-SBSFU (DM00414687 lub UM2262);
- **Integration guide** (DM00414677).

Istnieją adresy we Flashu, kolejność uruchamiania i oprogramowania sprzętowego aplikacji oraz użycie Ymodemu, jeśli to konieczne.

---

## 4. Sprawdź na tablicy

- Po flashowaniu **SBSFU_Boot** i **SBSFU_App** zgodnie z instrukcjami pakietu wykonaj **Reset**.
- Typowo: zielona dioda LED po pomyślnym uruchomieniu; gdy włączony jest UART - komunikaty bootloadera i aplikacji (parametry portu znajdziesz w instrukcji obsługi).
- Jeśli włączona jest funkcja Bezpieczna aktualizacja oprogramowania sprzętowego przez UART, dziennik może zawierać zaproszenie do pobrania nowego obrazu przez Ymodem.

---

## 5. Połączenie z naszym projektem

- **Nasz projekt** ([stm32_secure_boot](../readme.md)) - minimalny bootloader z własnym podpisem (ECDSA) i bez aktualizacji poprzez UART; jeden skrypt `./scripts/flash.sh` zbiera i flashuje wszystko.
- **X-CUBE-SBSFU** - kompletne rozwiązanie ST: weryfikacja podpisu (RSA/ECDSA), aktualizacja poprzez UART, dwa sloty, itp.; montaż i oprogramowanie sprzętowe zgodnie z instrukcjami pakietu (często poprzez CubeIDE).

Obie opcje można przechowywać na tej samej maszynie: nasz kod znajduje się w `stm32_secure_boot`, oficjalny znajduje się w rozpakowanym X-CUBE-SBSFU. Dla tej samej płyty NUCLEO-H743ZI przy przełączaniu się pomiędzy nimi wystarczy flashować odpowiedni obraz z 0x08000000 (oraz w razie potrzeby aplikację - zgodnie z instrukcją rozwiązania, z którego korzystasz).

Jeżeli po rozpakowaniu nie masz folderu dla NUCLEO-H743ZI, otwórz **Dokumenty** paczki i sprawdź listę obsługiwanych płyt w Instrukcji Obsługi; W przypadku niektórych płyt może być konieczne przeniesienie przykładu z innej płytki przy użyciu Przewodnika integracji (DM00414677).
