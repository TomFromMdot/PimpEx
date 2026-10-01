# {{NAME}} – a game on the PimpEx engine

Instructions for AI agents (Claude Code, Codex, Cursor...) and for people.
The whole game is text files, without a GUI editor.

## Project map

| File                             | Contains                                            | Specification          |
|----------------------------------|-----------------------------------------------------|------------------------|
| `pimpex.xml`                     | window, title, start scene                          | `docs/scene-xml.md`    |
| `scenes/<scene>/scene.xml`       | actors (GameObjects) = a name + components; archetypes; `mode="3d"` | `docs/scene-xml.md`, `docs/components.md` |
| `scenes/<scene>/scene.swap`      | **the scene logic** – procedures run every frame    | `docs/opcodes.md`      |
| `scenes/<scene>/library.swap`    | procedures pasted on demand (`PROC_ADD`)            | `docs/opcodes.md`      |
| `components.xml`                 | your own data components (e.g. Health)              | `docs/components.md`   |
| `scripts/behaviors/*.lua`        | actor behaviors (`<Behavior name="...">`)           | `docs/behaviors.md`    |
| `scripts/*.lua`                  | functions for `CALL` and `system()` systems         | `docs/lua-api.md`      |
| `gui/<name>.xml`                 | GUI: menus, pause, buttons (`GUI_SHOW name`)        | `docs/gui.md`          |
| `.pimpex/state.txt`              | a state dump of the running game (read-only)        | –                      |
| `.pimpex/runtime.log`            | errors, `LOG` output, changes made by the engine    | –                      |

Before you change something, read the matching specification in `docs/`. The
`.swap` language is small and strict. Do not invent instructions, components
or fields that are not there. `pimpex components .` lists every component.

## MCP tools

This project has a `.mcp.json`, so an MCP-compatible agent sees the
`pimpex_*` and `live_*` tools (docs/api.md):

- `pimpex_docs` – documentation (`opcodes`, `components`, `behaviors`, `gui`, …),
- `pimpex_check` – checks the project,
- `pimpex_run` – a headless run with scripted input, returns the state,
- `pimpex_scene_new` – a new scene from a template,
- `live_*` – reading and changing the game running in `pimpex dev`: state,
  fields, pasting procedures, keys, screenshots, Lua.

## The work loop

1. Change the files. Start a new scene from a template:
   `pimpex scene new level2 --template platformer2d` (list: `pimpex scene list`).
2. `pimpex check .` (or `pimpex_check`). Errors look like
   `file:line: error: message`, often with a hint ("did you mean …?").
   Fix every error.
3. `pimpex play . --headless --frames 120 --input "RIGHT:0-60 SPACE:30"`
   (or `pimpex_run`) runs the logic without a window with scripted keys.
   `--scene name` picks the scene.
4. Read `.pimpex/state.txt` (positions, components, variables, procedures)
   and `.pimpex/runtime.log`. Check that the game behaves as intended.
5. Check the looks with `pimpex play . --frames 60 --screenshot shot.png`
   or `live_screenshot`.

While `pimpex dev .` runs, saved files reach the game live and the `live_*`
tools change the game without a restart. A `PROC name ONCE` appended to
`scene.swap` runs once and disappears.

## Rules

- An actor = `<Actor name="..." x y w h tags="...">` + components inside
  (`<Rect>`, `<Collider>`, `<Camera>`, `<Behavior>`…). Write `Transform` and
  `Body` fields (`x`, `vx`, `gravity`…) on `<Actor>`.
- Draw grid levels (mazes, platforms) as an ASCII `<Tilemap>` with a
  character → archetype legend (docs/scene-xml.md). Do not write hundreds of
  `<Actor>`s.
- Sight and shooting: `self:look()`, `self:sees()`, `raycast()` (docs/behaviors.md).
- Keep game data in `components.xml` components or `<Var>`s (numbers), e.g. on
  an invisible `Game` actor.
- Mark groups of actors with tags and handle them with a system:
  `PROC coins FOR @coin` / `SELF` / `OTHER` or `system("@coin", ...)` in Lua.
- Movement with collisions: `Body` (vx, vy, gravity) + `<Collider/>` on both
  actors. Ready behaviors are in `scripts/behaviors/` (topdown_move,
  platformer_move, chase, patrol, spin, move3d, fps_controller).
- 2D: `x`, `y` is the centre, `y` grows down, velocity is in pixels per frame.
  3D (`<Scene mode="3d">`): `y` grows up, units are metres.
- HUD (score, lives): an actor with `space="screen"` and `<Text value="... {Game.coins}"/>`.
- Menus, pause and buttons: a `gui/<name>.xml` file (`<Button click="SCENE level">`),
  turned on with `GUI_SHOW name` or `<Scene gui="name">` (docs/gui.md).
  A headless click: `--input "CLICK@400/300:10"`.
- `.swap` has no arithmetic (`Player.x + 5` does not work). Use `ADD`, `MUL`
  or Lua.
- Do not edit files in `.pimpex/`. The engine creates them.
- Keep the project readable for a child: short names, short comments.
