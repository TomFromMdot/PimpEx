# PimpEx – The Game Engine

> **PimpEx** to silnik gier dla najmłodszego pokolenia twórców. Cała gra to
> pliki tekstowe. Nie ma edytora GUI: to, co dzieje się „pod maską”, widać
> w plikach i można to zmieniać w trakcie gry.

- **Twórca:** Tommy · **Start projektu:** 26.03.2026 · **Status:** wczesny rozwój (0.1)

```
pimpex init moja_gra     # nowy projekt z gotową mini-grą
pimpex dev moja_gra      # gra w oknie + hot-reload: zapisz plik → zmiana od razu
```

## Jak to działa

Gra składa się z trzech warstw:

| Warstwa         | Plik                         | Co opisuje                                              |
|-----------------|------------------------------|---------------------------------------------------------|
| **Model**       | `scenes/<scena>/scene.xml`   | aktorzy: pozycja, rozmiar, wygląd, zmienne; archetypy   |
| **Proces sceny**| `scenes/<scena>/scene.swap`  | procedury (`PROC`) wykonywane w każdej klatce, bez końca |
| **Logika Lua**  | `scripts/*.lua`              | bardziej złożone rzeczy, wołane z `.swap` przez `CALL`  |

```
# scene.swap
PROC ruch_gracza
  IF KEY LEFT  : MOVE Player -4 0
  IF KEY RIGHT : MOVE Player 4 0
END

PROC zbieranie
  IF TOUCH Player Coin : ADD Game.coins 1
  IF TOUCH Player Coin : CALL coin_collected
END
```

### Scena to nieskończony proces opcodów

Plik `scene.swap` to program pętli sceny, który działa przez cały czas gry.
Procedury można do niego **wklejać i z niego wycinać w trakcie gry**:

- człowiek albo agent AI dopisuje `PROC ... END` i zapisuje plik, a silnik
  podmienia program między klatkami;
- `PROC nazwa ONCE` wykonuje się raz, a potem silnik sam usuwa ją z pliku;
- `PROC_ADD nazwa` (albo `swap.add()` w Lua) wkleja gotową procedurę
  z `library.swap`, a `PROC_REMOVE` ją wycina.

Wszystkimi tymi zmianami zarządza **Boot**. Trzyma w pamięci dane sceny
(świat EnTT, program, bibliotekę, stan Lua) i jako jedyny zapisuje `.swap`:
atomowo i bez ruszania komentarzy ani reszty pliku. Jeśli w pliku jest błąd,
gra działa dalej na poprzedniej wersji, a błąd z numerem linii trafia do
`.pimpex/runtime.log`.

### Dwa wątki

```
wątek zasobów:  FileWatcher → parsowanie (.swap / XML / Lua) → Mailbox
wątek główny:   Mailbox → Boot (podmiana danych) → VM (PROC) → ruch → render (raylib)
```

Dane zawsze podmieniają się **między klatkami**, więc w trakcie klatki nic
nie zmienia się pod nogami.

## Komendy

| Komenda                                   | Co robi                                                   |
|-------------------------------------------|-----------------------------------------------------------|
| `pimpex init <nazwa> [--dir d]`           | nowy projekt z szablonu (gra „Zbieracz monet”)            |
| `pimpex check [projekt]`                  | sprawdza XML, `.swap` i Lua, błędy jako `plik:linia: opis`|
| `pimpex dev [projekt]`                    | gra z hot-reloadem                                        |
| `pimpex play [projekt]`                   | sama gra, zmiany `.swap` tylko w pamięci                  |
| `pimpex build [projekt] [--out dist]`     | paczka: silnik + gra + `graj.sh`                          |

Opcje `dev`/`play` dla testów i agentów AI:

- `--headless --frames N` – sama logika, bez okna
- `--input "RIGHT:0-60 SPACE:10"` – zaplanowane klawisze (z `--headless`)
- `--screenshot shot.png` – zrzut ostatniej klatki (z oknem i `--frames`)

Działająca gra zapisuje stan w `.pimpex/state.txt`: aktorów, ich pola,
wyliczone napisy i listę procedur.

## Agenci AI

PimpEx nie ma wbudowanego modelu. Jest za to zbudowany tak, żeby agent
(Claude Code, Codex, Cursor…) mógł od razu pisać gry. Każdy projekt z
`pimpex init` zawiera:

- `AGENTS.md` (i `CLAUDE.md`) – mapę projektu, pętlę pracy i zasady;
- `docs/` – dokładną specyfikację `.swap`, XML i API Lua;
- pętlę sprawdzania: `pimpex check` → `pimpex play --headless --input …` →
  `.pimpex/state.txt` i `runtime.log` → opcjonalnie `--screenshot`.

Spróbuj: `pimpex init gra && cd gra && claude "dodaj przeciwnika, który goni gracza"`.

## Dokumentacja

- [docs/opcodes.md](docs/opcodes.md) – język `.swap`
- [docs/scene-xml.md](docs/scene-xml.md) – `pimpex.xml` i `scene.xml`
- [docs/lua-api.md](docs/lua-api.md) – API skryptów Lua
- [examples/pong](examples/pong) – Pong: dwie sceny, komputer wklejany z biblioteki

## Budowanie silnika

Wymagania: CMake ≥ 3.28, Ninja, kompilator z C++23 (GCC 14+ / Clang 18+),
[vcpkg](https://vcpkg.io) (w `$VCPKG_ROOT` albo w `~/vcpkg`).

```
cmake --preset linux-debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/pimpex dev examples/pong
```

Zależności (vcpkg): raylib, EnTT, pugixml, Lua + sol2, CLI11, doctest.

## Struktura repozytorium

```
engine/core     Engine (pętla), Log, Mailbox, Diagnostic
engine/boot     Boot, ładowanie XML, SwapFile, FileWatcher, pimpex check
engine/vm       Lexer, Parser, Program, VM, klawisze i input
engine/scene    komponenty i World (EnTT)
engine/script   Lua (sol2): runtime i sprawdzanie skryptów
engine/render   raylib: Renderer, RaylibInput
engine/cli      komendy CLI (wzorzec Command) + wbudowany szablon
apps/pimpex     main()
templates/      szablon projektu dla `pimpex init`
docs/           specyfikacje (kopiowane też do każdego projektu)
examples/       przykładowe gry
tests/          testy doctest + testy end-to-end w ctest
```
