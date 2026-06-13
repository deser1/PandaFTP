# PandaFTP - Wtyczka FTP/SFTP dla Notepad++

PandaFTP to potężna, w pełni zintegrowana wtyczka do edytora Notepad++, umożliwiająca bezpośrednią edycję plików na zdalnych serwerach FTP oraz SFTP. Została zaprojektowana z myślą o wydajności i bezpieczeństwie, bazując na sprawdzonych bibliotekach takich jak libcurl oraz OpenSSL.

## 🌟 Główne funkcje

- **Obsługa wielu protokołów:** Wbudowane, natywne wsparcie dla standardowego FTP, bezpiecznego FTPS oraz szyfrowanego SFTP (przez SSH).
- **Edycja w locie (Live Edit):** Pobieraj pliki z serwera dwukrotnym kliknięciem. Gdy tylko wciśniesz `Ctrl+S` (Zapisz) w edytorze, wtyczka błyskawicznie i bez pytania wyśle zaktualizowany plik z powrotem na serwer w tle.
- **Menedżer Profili:** Zapisuj dane logowania do wielu serwerów. Hasła i ustawienia przechowywane są w formacie JSON w oficjalnym katalogu konfiguracyjnym Notepad++.
- **Inteligentne Zarządzanie Pamięcią:** Tymczasowe pliki są automatycznie usuwane z Twojego dysku twardego w ułamku sekundy po zamknięciu karty w Notepad++. Żadnego zaśmiecania systemu!
- **Wysyłanie nowych plików:** Utworzyłeś nowy plik lokalnie? Zaznacz folder na serwerze, kliknij "Wyślij", a wtyczka zrzuci plik na serwer i automatycznie podepnie go pod "Live Edit".
- **Drzewo Katalogów z Ikonami:** Przejrzysty widok struktury plików i folderów. Używa Twoich natywnych ikon z systemu Windows, przez co dokładnie wiesz, jakiego typu plik edytujesz.
- **Podtrzymywanie Połączenia:** Wtyczka nie loguje się od nowa przy każdej akcji. Trzyma aktywne gniazdo sieciowe, przez co operacje na plikach są błyskawiczne.
- **Szczegółowe Logowanie (Verbose):** Okienko na dole panelu dokładnie informuje o każdym etapie łączenia i komunikacji z serwerem FTP, bardzo pomocne przy diagnozowaniu problemów z siecią lub zaporami.

## 💻 Wymagania systemowe

- Notepad++ w wersji x64 lub x86
- System operacyjny Windows 10 / 11

## 🚀 Instalacja (Gotowa Wtyczka)

Jeśli posiadasz już skompilowany plik `PandaFTP.dll`:

1. Przejdź do katalogu instalacyjnego Notepad++ (zazwyczaj `C:\Program Files\Notepad++\plugins\`).
2. Utwórz nowy folder o nazwie `PandaFTP`.
3. Wklej do niego plik `PandaFTP.dll`.
   *(Upewnij się, że używasz wersji bitowej odpowiadającej Twojemu edytorowi Notepad++ - x64 dla 64-bit lub Win32 dla 32-bit).*
4. Uruchom ponownie Notepad++.
5. W górnym menu edytora wybierz **Wtyczki -> PandaFTP -> Panel PandaFTP**.

## 🛠️ Kompilacja ze źródeł (Dla Deweloperów)

Projekt jest skonfigurowany pod **Visual Studio 2019** (Platform Toolset v142) z użyciem **MSBuild**.
Wszystkie niezbędne biblioteki kryptograficzne (OpenSSL, libssh2, libcurl, zlib) są konsolidowane **statycznie** (Static Linking) do jednego pliku `.dll` dla maksymalnej kompatybilności i wygody dystrybucji.

1. Sklonuj repozytorium do dowolnego folderu na dysku (np. `D:\Projekty\FTP_Notepad++`).
2. Otwórz plik rozwiązania: `vs.proj\PandaFTP.vcxproj` w Visual Studio 2019.
3. Wybierz konfigurację budowania:
   - Configuration: **Release**
   - Platform: **x64** (dla 64-bitowego Notepad++) lub **Win32** (dla 32-bitowego)
4. W prawym panelu (Eksplorator rozwiązań) kliknij prawym przyciskiem myszy na projekt i wybierz **Kompiluj** (Build).
5. Gotowy plik znajdziesz w podkatalogu `bin64\` lub `bin\`.

### Zależności (Zaszyte w konfiguracji projektu)
Projekt korzysta z natywnych bibliotek Windows: `Bcrypt.lib`, `Secur32.lib`, `Iphlpapi.lib`, `Ws2_32.lib`, `Wldap32.lib`, `Crypt32.lib`, `Normaliz.lib` oraz API `Shell32`.

## 📜 Licencja i Twórcy

- **Wydawca:** Domek Software
- **Wersja:** 1.0.0
- **Licencja:** Oprogramowanie typu Open Source. Zbudowane na podstawie oficjalnego szablonu Npp Plugin Template.

---
*Dziękujemy za korzystanie z PandaFTP! Jeśli wtyczka oszczędza Twój czas, podziel się nią z innymi programistami!*
