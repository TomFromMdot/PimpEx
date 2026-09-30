# API na żywo i serwer MCP (dla agentów AI)

Działającą grę można czytać i zmieniać z zewnątrz, na dwa sposoby:

1. **Serwer MCP** – `pimpex mcp` – dla agentów zgodnych z MCP (Claude Code i inni).
   Projekt z `pimpex init` ma już plik `.mcp.json`, więc agent od razu
   widzi narzędzia PimpEx.
2. **JSON-RPC przez HTTP** – dla skryptów, testów i własnych narzędzi.

## Serwer MCP

```json
// .mcp.json w katalogu projektu (tworzy go pimpex init)
{ "mcpServers": { "pimpex": { "command": "/pełna/ścieżka/do/pimpex", "args": ["mcp", "."] } } }
```

`pimpex init` wpisuje pełną ścieżkę do binarki, która utworzyła projekt.
Po przeniesieniu projektu na inny komputer zmień ją, np. na samo `pimpex`,
jeśli jest w `PATH`.

Narzędzia offline (działają zawsze):

| Narzędzie              | Co robi                                                          |
|------------------------|------------------------------------------------------------------|
| `pimpex_check`         | sprawdza cały projekt, zwraca błędy `plik:linia: opis`           |
| `pimpex_run`           | gra bez okna: `frames`, `input` (np. `RIGHT:0-60 SPACE:10`), `scene`; zwraca state.txt i log |
| `pimpex_docs`          | dokumentacja: `opcodes`, `components`, `behaviors`, `lua-api`, `scene-xml`, `api` |
| `pimpex_scene_new`     | nowa scena z szablonu (`template`: empty2d, topdown2d, platformer2d, empty3d, arena3d, starter2d) |

Narzędzia na żywo (wymagają uruchomionego `pimpex dev`):

| Narzędzie            | Co robi                                                   |
|----------------------|-----------------------------------------------------------|
| `live_state`         | aktorzy i procedury działającej gry (`selector`, `brief`) |
| `live_actor`         | wszystkie komponenty jednego aktora                       |
| `live_set`           | ustawia pole aktora (`name`, `field`, `value`)            |
| `live_spawn`         | tworzy aktora z archetypu                                 |
| `live_destroy`       | usuwa aktora                                              |
| `live_swap_paste`    | wkleja `PROC ... END` do działającego `scene.swap`         |
| `live_press`         | wciska klawisze na N klatek                               |
| `live_control`       | `pause`, `resume`, `step` (z `frames`), `quit`            |
| `live_screenshot`    | zrzut ekranu (obraz PNG)                                  |
| `live_log`           | ostatnie linie dziennika                                  |
| `live_lua`           | wykonuje kod Lua w grze                                   |
| `live_scene`         | przełącza scenę                                           |

Bez okna (serwer, CI, agent bez ekranu): `pimpex dev --headless` działa
w swoim tempie, dopóki nie dostanie `quit` albo Ctrl+C.

## JSON-RPC przez HTTP

`pimpex dev` (bez `--no-api`) albo `pimpex play --api` uruchamia serwer
na `127.0.0.1` pod losowym portem i zapisuje `.pimpex/api.json`:

```json
{ "url": "http://127.0.0.1:44551", "port": 44551, "token": "…", "pid": 1234, "project": "…" }
```

Każde żądanie to `POST /rpc` z nagłówkiem `Authorization: Bearer <token>`:

```sh
curl -s -X POST "$URL/rpc" -H "Authorization: Bearer $TOKEN" \
  -d '{"jsonrpc":"2.0","id":1,"method":"actor.set","params":{"name":"Player","field":"x","value":100}}'
```

`GET /methods` zwraca listę metod. Metody wykonują się **między klatkami**
na wątku gry. Odpowiedź to zwykły JSON-RPC 2.0 (`result` albo `error`).

| Metoda                   | Parametry                       | Wynik / działanie |
|--------------------------|---------------------------------|-------------------|
| `ping`                   |                                 | scena, klatka, pauza |
| `state.get`              | `selector?`, `brief?`           | scena, tryb, klatka, procedury, aktorzy (z komponentami) |
| `actors.list`            | `selector?`                     | nazwy aktorów (`"Health"`, `"@enemy"`) |
| `actor.get`              | `name`                          | komponenty, zmienne, tagi, zachowania |
| `actor.set`              | `name`, `field`, `value`        | `"x"`, `"vx"`, `"Health.hp"`, `"Text.value"`, zmienna |
| `actor.spawn`            | `archetype`, `name?`, `x?`, `y?`, `z?` | nowy aktor |
| `actor.destroy`          | `name`                          | |
| `actor.add_component`    | `name`, `component`, `values?`  | np. `{"component":"Body","values":{"vx":2}}` |
| `actor.remove_component` | `name`, `component`             | |
| `swap.get`               |                                 | tekst `scene.swap`, procedury, biblioteka |
| `swap.paste`             | `source`                        | wkleja kod (PROC o tej samej nazwie jest podmieniana) |
| `swap.add` / `swap.remove` | `name`                        | PROC z `library.swap` / usunięcie z `scene.swap` |
| `scene.load`             | `name`                          | zmiana sceny |
| `scene.list`             |                                 | sceny projektu |
| `input.press`            | `keys`, `frames?`               | `{"keys":["SPACE"],"frames":10}` |
| `game.pause` / `game.resume` |                             | logika stoi / rusza (rysowanie trwa) |
| `game.step`              | `frames?`                       | N klatek logiki w czasie pauzy |
| `game.quit`              |                                 | zamyka grę |
| `screenshot`             |                                 | `png_base64` (wymaga okna) |
| `log.tail`               | `lines?`                        | ostatnie linie dziennika |
| `components.list`        |                                 | schemat komponentów |
| `lua.eval`               | `code`                          | wynik kodu Lua, np. `get("Player","x")` |
| `check`                  |                                 | wynik `pimpex check` |

Bezpieczeństwo: serwer słucha tylko na `127.0.0.1` i wymaga tokenu z pliku,
który może przeczytać tylko ktoś z dostępem do katalogu projektu.
