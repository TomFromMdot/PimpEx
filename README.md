# PimpEx – The Game Engine

> **PimpEx** to silnik gier dla najmłodszego pokolenia twórców. Cała gra to
> pliki tekstowe. Nie ma edytora GUI: to, co dzieje się „pod maską”, widać
> w plikach i można to zmieniać w trakcie gry.

- **Twórca:** Tommy · **Start projektu:** 26.03.2026 · **Status:** wczesny rozwój (0.2)

```
pimpex init moja_gra     # nowy projekt z gotową mini-grą
pimpex dev moja_gra      # gra w oknie + hot-reload: zapisz plik → zmiana od razu
```

## Jak to działa

Gra składa się z czterech warstw. Żadna nie wymaga kompilowania silnika:

| Warstwa           | Plik                          | Co opisuje                                                  |
|-------------------|-------------------------------|-------------------------------------------------------------|
| **Aktorzy (ECS)** | `scenes/<scena>/scene.xml`    | GameObjecty = nazwa + komponenty (wygląd, ruch, kolizje, kamera, światło, dane) |
| **Dane**          | `components.xml`              | własne komponenty, np. `Health` z polem `hp`                |
| **Proces sceny**  | `scenes/<scena>/scene.swap`   | procedury (`PROC`) wykonywane w każdej klatce, bez końca    |
| **Zachowania**    | `scripts/behaviors/*.lua`, `scripts/*.lua` | Lua: `<Behavior>` na aktorze, `system()`, funkcje dla `CALL` |

```xml
<!-- scene.xml -->
<Actor name="Ghost" x="400" y="300" tags="enemy">
  <Circle color="różowy"/>
  <Collider kind="trigger"/>
  <Health hp="3"/>
  <Behavior name="chase" target="Player" speed="1.5"/>
</Actor>
```

```
# scene.swap – system dla wszystkich wrogów
PROC wrogowie FOR @enemy
  IF TOUCH SELF Player : ADD Game.lives -1
  IF SELF.Health.hp <= 0 : DESTROY SELF
END
```

### Sceny 2D i 3D

`<Scene mode="3d">` włącza tryb 3D. Do dyspozycji są bryły (`Cube`, `Sphere`,
`Cylinder`, `Plane`), modele `.glb`/`.obj`, światła (`Light`), kamera
(`fixed`, `follow`, `orbit`, `fps`) i fizyka AABB z grawitacją. W 2D są
kształty, obrazki, kamera śledząca i HUD (`space="screen"`). Gotowe sceny
tworzy się jedną komendą:

```
pimpex scene new poziom2 --template platformer2d   # empty2d, topdown2d, platformer2d, empty3d, arena3d, starter2d
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

### Wątki i klatka

```
wątek zasobów:  FileWatcher → parsowanie (.swap / XML / Lua) → Mailbox
wątek API:      HTTP JSON-RPC (127.0.0.1) → Mailbox
wątek główny:   Mailboxy → Boot → .swap (VM) → zachowania i systemy Lua → fizyka → on_touch → render
```

Dane i polecenia z API podmieniają się zawsze **między klatkami**, więc
w trakcie klatki nic nie zmienia się pod nogami.

## Komendy

| Komenda                                        | Co robi                                                   |
|------------------------------------------------|-----------------------------------------------------------|
| `pimpex init <nazwa> [--template t]`           | nowy projekt (z `.mcp.json`, `AGENTS.md`, docs i zachowaniami) |
| `pimpex scene new <nazwa> [--template t]`      | nowa scena z szablonu; `pimpex scene list` – szablony     |
| `pimpex check [projekt]`                       | sprawdza XML, `.swap`, Lua i komponenty (`plik:linia: opis`) |
| `pimpex components [projekt] [--markdown]`     | wszystkie komponenty i ich pola                           |
| `pimpex dev [projekt]`                         | gra z hot-reloadem i API na żywo                          |
| `pimpex play [projekt]`                        | sama gra (zmiany `.swap` tylko w pamięci)                 |
| `pimpex build [projekt] [--out dist]`          | paczka: silnik + gra + `graj.sh`                          |
| `pimpex mcp [projekt]`                         | serwer MCP dla agentów AI                                 |

Opcje `dev` i `play`:

- `--scene nazwa` – scena startowa;
- `--headless` – sama logika, bez okna (z `--frames N`; `dev --headless`
  działa do `quit`);
- `--input "RIGHT:0-60 SPACE:10"` – zaplanowane klawisze;
- `--screenshot shot.png` – zrzut ostatniej klatki;
- `--no-api` / `--api` – API na żywo.

## Agenci AI

PimpEx nie ma wbudowanego modelu. Za to każdy projekt jest od razu gotowy
dla agenta:

- **`.mcp.json`** – Claude Code (i inni klienci MCP) dostaje narzędzia
  `pimpex_check`, `pimpex_run`, `pimpex_docs`, `pimpex_scene_new` oraz `live_*`
  do działającej gry: stan, pola aktorów, wklejanie procedur, klawisze,
  zrzut ekranu, Lua;
- **API na żywo** – JSON-RPC na `127.0.0.1` (port i token w
  `.pimpex/api.json`) dla skryptów i własnych narzędzi (docs/api.md);
- **`AGENTS.md` / `CLAUDE.md`** – mapa projektu, pętla pracy, zasady;
- **pętla sprawdzania** – `check` → `play --headless --input` →
  `state.txt` / `runtime.log` → `--screenshot`.

Spróbuj: `pimpex init gra --template arena3d && cd gra && claude "dodaj trzeciego wroga, który patroluje przy ścianie"`.

## Dokumentacja

- [docs/opcodes.md](docs/opcodes.md) – język `.swap`
- [docs/scene-xml.md](docs/scene-xml.md) – `pimpex.xml` i `scene.xml`
- [docs/lua-api.md](docs/lua-api.md) – API skryptów Lua
- [docs/components.md](docs/components.md) – komponenty ECS (generowane z kodu)
- [docs/behaviors.md](docs/behaviors.md) – zachowania i systemy
- [docs/api.md](docs/api.md) – API na żywo i serwer MCP
- [examples/pong](examples/pong) – Pong: dwie sceny, komputer wklejany z biblioteki
- [examples/wolf3d](examples/wolf3d) – Wolf 3D: mapa ASCII, strażnicy z wzrokiem (raycast), drzwi, znajdźki

## Budowanie silnika

Wymagania: CMake ≥ 3.28, Ninja, kompilator z C++23 (GCC 14+ / Clang 18+),
[vcpkg](https://vcpkg.io) (w `$VCPKG_ROOT` albo w `~/vcpkg`).

```
cmake --preset linux-debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/pimpex dev examples/pong
```

Zależności (vcpkg): raylib (+ GLFW), EnTT, pugixml, Lua + sol2, CLI11,
cpp-httplib, nlohmann-json, doctest. Czcionka: Anonymous Pro (SIL OFL,
`third_party/fonts/`).

## Struktura repozytorium

```
engine/core     Engine (pętla), Log, Mailbox, Diagnostic
engine/boot     Boot, ładowanie XML, SwapFile, FileWatcher, pimpex check
engine/vm       Lexer, Parser, Program, VM, klawisze i input
engine/scene    ComponentRegistry, komponenty, World (EnTT), fizyka, docs komponentów
engine/script   Lua (sol2): runtime, zachowania, systemy, sprawdzanie skryptów
engine/render   raylib: Renderer 2D/3D, shader światła, czcionka, RaylibInput
engine/api      API na żywo (JSON-RPC/HTTP) i serwer MCP
engine/cli      komendy CLI (wzorzec Command) + szablony
apps/pimpex     main()
templates/      szablony: projekt, sceny, biblioteka zachowań
docs/           specyfikacje (kopiowane też do każdego projektu)
examples/       przykładowe gry
tests/          testy doctest + testy end-to-end w ctest
```
