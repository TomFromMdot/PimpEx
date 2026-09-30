# {{NAME}} – gra na silniku PimpEx

Instrukcje dla agentów AI (Claude Code, Codex, Cursor...) i dla ludzi.
Cała gra to pliki tekstowe. Nie ma edytora GUI, więc pracuje się wyłącznie
na plikach poniżej.

## Mapa projektu

| Plik                             | Co zawiera                                         | Specyfikacja           |
|----------------------------------|----------------------------------------------------|------------------------|
| `pimpex.xml`                     | okno, tytuł, scena startowa                        | `docs/scene-xml.md`    |
| `scenes/<scena>/scene.xml`       | aktorzy: pozycja, rozmiar, wygląd, zmienne; archetypy | `docs/scene-xml.md` |
| `scenes/<scena>/scene.swap`      | **logika sceny** – procedury wykonywane co klatkę  | `docs/opcodes.md`      |
| `scenes/<scena>/library.swap`    | procedury wklejane na żądanie (`PROC_ADD`)         | `docs/opcodes.md`      |
| `scripts/*.lua`                  | złożona logika wołana przez `CALL`                 | `docs/lua-api.md`      |
| `.pimpex/state.txt`              | zrzut stanu działającej gry (tylko do odczytu)     | –                      |
| `.pimpex/runtime.log`            | błędy, `LOG`, zmiany wprowadzone przez silnik      | –                      |

Zanim coś zmienisz, przeczytaj odpowiednią specyfikację w `docs/`. Język
`.swap` jest mały i ścisły. Nie wymyślaj instrukcji, których tam nie ma.

## Pętla pracy

1. Zmień pliki.
2. `pimpex check .` sprawdza wszystko bez uruchamiania gry. Błędy mają
   format `plik:linia: błąd: opis`. Popraw wszystkie błędy, a ostrzeżenia
   przeczytaj.
3. `pimpex play . --headless --frames 120 --input "RIGHT:0-60 SPACE:30"`
   uruchamia logikę bez okna, z zaplanowanymi klawiszami (`KLAWISZ:od-do`
   albo `KLAWISZ:klatka`).
4. Przeczytaj `.pimpex/state.txt` (pozycje, zmienne, napisy, lista
   procedur) i `.pimpex/runtime.log` (błędy wykonania, `LOG`). Sprawdź,
   czy gra zachowuje się tak, jak chciałeś.
5. Jeśli możesz oglądać obrazy:
   `pimpex play . --frames 60 --screenshot shot.png` zapisuje zrzut ekranu.

Gdy użytkownik ma uruchomione `pimpex dev .`, każdy zapisany plik trafia do
gry w locie. Wtedy zamiast restartu możesz:

- dopisać do `scene.swap` procedurę `PROC nazwa ONCE`. Silnik wykona ją raz
  i sam usunie z pliku (dobre do poleceń typu „daj graczowi 10 monet”);
- zmienić `scene.xml`. Aktorzy zachowują pozycje, a zmienia się ich wygląd.

## Zasady

- Nazwy aktorów, archetypów i procedur piszemy bez spacji i bez `.`, `/`, `:`.
- Każdy aktor, którego używa `scene.swap`, musi istnieć w `scene.xml` albo
  powstać przez `SPAWN`. Archetypy do `SPAWN` definiuje się w `scene.xml`.
- Stan gry (punkty, życia) trzymaj w zmiennych `<Var>` na aktorach, np. na
  niewidocznym aktorze `Game`.
- Pozycja `x`, `y` to **środek** aktora. `y` rośnie w dół. Prędkość
  `vx`, `vy` to piksele na klatkę (60 klatek = 1 sekunda).
- Nie ma wyrażeń arytmetycznych w `.swap` (`Player.x + 5` nie zadziała).
  Użyj `ADD`, `MUL` albo funkcji Lua.
- Proste reguły (ruch, kolizje, punkty) pisz w `.swap`. Pętle, listy
  i obliczenia przenieś do Lua.
- Nie edytuj plików w `.pimpex/`. Tworzy je silnik.
- Komentarze i nazwy mogą być po polsku. Projekt ma być czytelny dla dziecka.
