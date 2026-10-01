# Live API and MCP server (for AI agents)

A running game can be read and changed from outside in two ways:

1. **The MCP server** – `pimpex mcp` – for MCP-compatible agents (Claude Code
   and others). A project made with `pimpex init` already has a `.mcp.json`,
   so the agent sees the PimpEx tools right away.
2. **JSON-RPC over HTTP** – for scripts, tests and your own tools.

## The MCP server

```json
// .mcp.json in the project directory (created by pimpex init)
{ "mcpServers": { "pimpex": { "command": "/full/path/to/pimpex", "args": ["mcp", "."] } } }
```

`pimpex init` writes the full path of the binary that created the project.
After moving the project to another computer, change it, e.g. to plain
`pimpex` if it is in `PATH`.

Offline tools (always available):

| Tool                   | What it does                                                     |
|------------------------|------------------------------------------------------------------|
| `pimpex_check`         | checks the whole project, returns `file:line: error: message`    |
| `pimpex_run`           | a headless run: `frames`, `input` (e.g. `RIGHT:0-60 SPACE:10 CLICK@400/300:20`), `scene`; returns state.txt and the log |
| `pimpex_docs`          | docs: `opcodes`, `components`, `behaviors`, `lua-api`, `scene-xml`, `gui`, `api` |
| `pimpex_scene_new`     | a new scene from a template (`template`: empty2d, topdown2d, platformer2d, empty3d, arena3d, starter2d) |

Live tools (need a running `pimpex dev`):

| Tool                 | What it does                                              |
|----------------------|-----------------------------------------------------------|
| `live_state`         | actors and procedures of the running game (`selector`, `brief`) |
| `live_actor`         | all components of one actor                               |
| `live_set`           | sets an actor field (`name`, `field`, `value`)            |
| `live_spawn`         | creates an actor from an archetype                        |
| `live_destroy`       | removes an actor                                          |
| `live_swap_paste`    | pastes `PROC ... END` into the running `scene.swap`       |
| `live_press`         | holds keys for N frames                                   |
| `live_control`       | `pause`, `resume`, `step` (with `frames`), `quit`         |
| `live_screenshot`    | a screenshot (PNG image)                                  |
| `live_log`           | the last log lines                                        |
| `live_lua`           | runs Lua code in the game (e.g. `gui.show("pause")`)      |
| `live_scene`         | switches the scene                                        |

Without a window (a server, CI, an agent without a screen):
`pimpex dev --headless` runs at its own pace until it gets `quit` or Ctrl+C.

## JSON-RPC over HTTP

`pimpex dev` (without `--no-api`) or `pimpex play --api` starts a server on
`127.0.0.1` on a random port and writes `.pimpex/api.json`:

```json
{ "url": "http://127.0.0.1:44551", "port": 44551, "token": "…", "pid": 1234, "project": "…" }
```

Every request is a `POST /rpc` with an `Authorization: Bearer <token>` header:

```sh
curl -s -X POST "$URL/rpc" -H "Authorization: Bearer $TOKEN" \
  -d '{"jsonrpc":"2.0","id":1,"method":"actor.set","params":{"name":"Player","field":"x","value":100}}'
```

`GET /methods` lists the methods. Methods run **between frames** on the game
thread. Responses are plain JSON-RPC 2.0 (`result` or `error`).

| Method                   | Parameters                      | Result / effect |
|--------------------------|---------------------------------|-----------------|
| `ping`                   |                                 | scene, frame, pause |
| `state.get`              | `selector?`, `brief?`           | scene, mode, frame, procedures, actors (with components) |
| `actors.list`            | `selector?`                     | actor names (`"Health"`, `"@enemy"`) |
| `actor.get`              | `name`                          | components, variables, tags, behaviors |
| `actor.set`              | `name`, `field`, `value`        | `"x"`, `"vx"`, `"Health.hp"`, `"Text.value"`, a variable |
| `actor.spawn`            | `archetype`, `name?`, `x?`, `y?`, `z?` | a new actor |
| `actor.destroy`          | `name`                          | |
| `actor.add_component`    | `name`, `component`, `values?`  | e.g. `{"component":"Body","values":{"vx":2}}` |
| `actor.remove_component` | `name`, `component`             | |
| `swap.get`               |                                 | `scene.swap` text, procedures, the library |
| `swap.paste`             | `source`                        | pastes code (a PROC with the same name is replaced) |
| `swap.add` / `swap.remove` | `name`                        | a PROC from `library.swap` / removal from `scene.swap` |
| `scene.load`             | `name`                          | switches the scene |
| `scene.list`             |                                 | the project's scenes |
| `input.press`            | `keys`, `frames?`               | `{"keys":["SPACE"],"frames":10}` |
| `game.pause` / `game.resume` |                             | logic stops / runs (drawing continues) |
| `game.step`              | `frames?`                       | N logic frames while paused |
| `game.quit`              |                                 | quits the game |
| `screenshot`             |                                 | `png_base64` (needs a window) |
| `log.tail`               | `lines?`                        | the last log lines |
| `components.list`        |                                 | the component schema |
| `lua.eval`               | `code`                          | the result of Lua code, e.g. `get("Player","x")` |
| `check`                  |                                 | the result of `pimpex check` |

Security: the server listens only on `127.0.0.1` and needs the token from a
file that only someone with access to the project directory can read.
