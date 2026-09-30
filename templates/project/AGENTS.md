# {{NAME}} – gra na silniku PimpEx

Instrukcje dla agentów AI (Claude Code, Codex, Cursor...) i dla ludzi.
Cała gra to pliki tekstowe, bez edytora GUI.

## Mapa projektu

| Plik                             | Co zawiera                                          | Specyfikacja           |
|----------------------------------|-----------------------------------------------------|------------------------|
| `pimpex.xml`                     | okno, tytuł, scena startowa                         | `docs/scene-xml.md`    |
| `scenes/<scena>/scene.xml`       | aktorzy (GameObjecty) = nazwa + komponenty; archetypy; `mode="3d"` | `docs/scene-xml.md`, `docs/components.md` |
| `scenes/<scena>/scene.swap`      | **logika sceny** – procedury wykonywane co klatkę   | `docs/opcodes.md`      |
| `scenes/<scena>/library.swap`    | procedury wklejane na żądanie (`PROC_ADD`)          | `docs/opcodes.md`      |
| `components.xml`                 | własne komponenty danych (np. Health)               | `docs/components.md`   |
| `scripts/behaviors/*.lua`        | zachowania aktorów (`<Behavior name="...">`)        | `docs/behaviors.md`    |
| `scripts/*.lua`                  | funkcje dla `CALL` i systemy `system()`             | `docs/lua-api.md`      |
| `.pimpex/state.txt`              | zrzut stanu działającej gry (tylko do odczytu)      | –                      |
| `.pimpex/runtime.log`            | błędy, `LOG`, zmiany wprowadzone przez silnik       | –                      |

Zanim coś zmienisz, przeczytaj odpowiednią specyfikację w `docs/`. Język
`.swap` jest mały i ścisły. Nie wymyślaj instrukcji, komponentów ani pól,
których tam nie ma. Pełną listę komponentów daje `pimpex components .`.

## Narzędzia MCP

Ten projekt ma `.mcp.json`, więc agent zgodny z MCP widzi narzędzia `pimpex_*`
i `live_*` (docs/api.md):

- `pimpex_docs` – dokumentacja (`opcodes`, `components`, `behaviors`, …),
- `pimpex_check` – sprawdzenie projektu,
- `pimpex_run` – gra bez okna z klawiszami, zwraca stan,
- `pimpex_scene_new` – nowa scena z szablonu,
- `live_*` – czytanie i zmiana gry uruchomionej w `pimpex dev`: stan, pola,
  wklejanie procedur, klawisze, zrzut ekranu, Lua.

## Pętla pracy

1. Zmień pliki. Nową scenę zacznij od szablonu:
   `pimpex scene new poziom2 --template platformer2d` (lista: `pimpex scene list`).
2. `pimpex check .` (albo `pimpex_check`). Błędy mają format
   `plik:linia: błąd: opis` i często podpowiedź („czy chodziło o …?”).
   Popraw wszystkie błędy.
3. `pimpex play . --headless --frames 120 --input "RIGHT:0-60 SPACE:30"`
   (albo `pimpex_run`) uruchamia logikę bez okna z zaplanowanymi klawiszami.
   `--scene nazwa` wybiera scenę.
4. Przeczytaj `.pimpex/state.txt` (pozycje, komponenty, zmienne, procedury)
   i `.pimpex/runtime.log`. Sprawdź, czy gra zachowuje się tak, jak chciałeś.
5. Wygląd sprawdzisz przez `pimpex play . --frames 60 --screenshot shot.png`
   albo `live_screenshot`.

Gdy działa `pimpex dev .`, zapisane pliki trafiają do gry w locie, a narzędzia
`live_*` zmieniają grę bez restartu. `PROC nazwa ONCE` dopisana do
`scene.swap` wykona się raz i zniknie.

## Zasady

- Aktor = `<Actor name="..." x y w h tags="...">` + komponenty w środku
  (`<Rect>`, `<Collider>`, `<Camera>`, `<Behavior>`…). Pola `Transform`
  i `Body` (`x`, `vx`, `gravity`…) pisz na `<Actor>`.
- Dane gry trzymaj w komponentach z `components.xml` albo w `<Var>` (liczby),
  np. na niewidocznym aktorze `Game`.
- Grupy aktorów oznaczaj tagami i obsługuj systemem:
  `PROC monety FOR @coin` / `SELF` / `OTHER` albo `system("@coin", ...)` w Lua.
- Ruch z kolizjami: `Body` (vx, vy, gravity) + `<Collider/>` na obu aktorach.
  Gotowe zachowania są w `scripts/behaviors/` (topdown_move, platformer_move,
  chase, patrol, spin, move3d, fps_controller).
- 2D: `x`, `y` to środek, `y` rośnie w dół, prędkość w pikselach na klatkę.
  3D (`<Scene mode="3d">`): `y` rośnie w górę, jednostki to metry.
- HUD (wynik, życia): aktor z `space="screen"` i `<Text value="... {Game.coins}"/>`.
- W `.swap` nie ma arytmetyki (`Player.x + 5` nie zadziała). Użyj `ADD`,
  `MUL` albo Lua.
- Nie edytuj plików w `.pimpex/`. Tworzy je silnik.
- Komentarze i nazwy mogą być po polsku. Projekt ma być czytelny dla dziecka.
