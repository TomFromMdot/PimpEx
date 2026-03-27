# PimpEx - The Game Engine

> **PimpEx** to silnik gier zaprojektowany z myślą o najmłodszym pokoleniu twórców. Naszym głównym celem jest dostarczenie narzędzia, którego API oraz architektura będą w pełni zrozumiałe nawet dla najmniej doświadczonych programistów. 

Domyślnie projekt wykorzystuje polskie komentarze, aby obniżyć próg wejścia dla lokalnych początkujących (w przyszłości planowane jest tłumaczenie na język angielski).

---

## 📊 Informacje o projekcie
- **Nazwa projektu:** PimpEx
- **Data utworzenia:** 26.03.2026
- **Twórca:** Tommy
- **Status:** Wczesny rozwój / Architektura

---

## 🎯 Główny cel (The Goat)

Dostarczenie silnika do produkcji gier, który umożliwia stworzenie pełnoprawnej gry bez ścisłego wymogu posiadania wbudowanego GUI edytora. 

**Jak to możliwe?**
Wyobraź sobie grę stworzoną na systemie operacyjnym, który nie wymaga zaawansowanej grafiki. Kluczem jest "Otwartość" i odpowiednia architektura. PimpEx pozwala na dokładną manipulację stanem silnika i gry poprzez pliki i skrypty, ucząc użytkowników, co tak naprawdę dzieje się "pod maską", a nie tylko jak klikać trójkąty w okienku edytora.

---

## 🏗️ Architektura i Warstwy

Aby uprościć proces tworzenia gier (wzorując się nieco na podejściu znanym z gier mobilnych), projekt dzieli warstwy produktu na trzy główne filary. Daje to prosty w implementacji i zrozumieniu mechanizm produkcyjny.

### 1. Model (Actor)
- **Actor** to podstawowy archetyp w naszym silniku.
- Dane aktora są ładowane z modelu przechowywanego w plikach projektu.
- Model zawiera informacje o tym, z jakich **Komponentów** składa się dany archetyp oraz jakie wartości mają zostać ustawione na start.
- W ten sam sposób można zapisać wartości z powrotem do pliku.
- **Hot-reloading:** Silnik daje możliwość przeładowania tych plików w locie, inicjując wartości na nowo (lub nie, w zależności od preferencji użytkownika).

### 2. Logika (Skrypty Lua)
- Całkowicie oddzielamy logikę zachowań aktorów od jądra silnika, przenosząc ją do języka **Lua**.
- Model w silniku definiuje komponent `Scriptable`, który działa jak hiperłącze do skryptu będącego częścią projektu gry. Dzięki temu modyfikacja zachowań obiektów jest szybka i bezbolesna.

### 3. Kontroler (Silnik C++)
- Składa się z plików takich jak `Controller.cpp` i `Controller.h`.
- To tutaj znajduje się twarda logika naszej sceny, zachowań silnika oraz zarządzania pamięcią.

---

## 🛠️ Założenia techniczne i wyzwania

- **Zrozumiałość dla dzieci:** Chcemy, aby dziecko analizujące przykładowy projekt było w stanie domyślić się mechaniki działania gry, poprawnie ją zinterpretować i z powodzeniem modyfikować.
- **Solidna Architektura:** Projekt kładzie ogromny nacisk na poprawne relacje obiektów (zasady SOLID i rygor architektoniczny).
- **Zarządzanie Pamięcią i Wydajność:** Priorytetem (i największym wyzwaniem) jest ciągła optymalizacja pod kątem sprzętowym i pamięciowym, aby silnik działał płynnie i stabilnie, niezależnie od poziomu doświadczenia użytkownika końcowego.
