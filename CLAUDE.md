# PimpEx – working on the engine

A game engine for kids where the whole game is text. The vision and
architecture are in `README.md`, the game file formats in `docs/`.

## Building and testing

```
cmake --preset linux-debug          # vcpkg from $VCPKG_ROOT or ~/vcpkg
cmake --build build
ctest --test-dir build --output-on-failure
```

- Unit tests (doctest) are in `tests/*.cpp`. VM, Boot and Lua are tested
  without a window (`vm::ManualInput`, `BootFixture` in `tests/test_boot.cpp`).
- End-to-end tests in `CMakeLists.txt` check the template (`pimpex init`) and
  every directory in `examples/` with `pimpex check` and `play --headless`.
- For a live look: `./build/pimpex play examples/pong --frames 60 --screenshot shot.png`.

## Rules

- C++23. Code, comments, docs and user-facing messages are in **English**.
  Comment only what the code cannot say (why, workarounds, pitfalls). Errors
  are always `file:line: error: message` (`Diagnostic`).
- Only `render/` and `core/Engine.cpp` know raylib. VM, Boot and World must
  work without a window.
- Only `boot::Boot` writes `scene.swap`, always through `write_file_atomic`
  and after `expect()` on the watcher.
- A new built-in component: a struct in `scene/Components.hpp` + a descriptor
  (`Binder`) in `ComponentRegistry::with_builtins()`. The loader, `.swap`,
  Lua, the API and the dump then just work. Then drawing (`render/`) or logic
  (`scene/Physics.cpp`), regenerating `docs/components.md`
  (`pimpex components --markdown > docs/components.md`, ctest checks it) and
  tests in `tests/test_components.cpp`.
- A new API method: `api/LiveApi.cpp` (`dispatch` + the `Methods` table),
  `docs/api.md`, a test in `tests/test_api.cpp`. If agents should have it,
  also a tool in `api/McpServer.cpp` (`tools_list` + `call_tool`) and
  `tests/test_mcp.cpp`.
- The MCP server writes the protocol to stdout. Everything else must go to
  stderr (`McpServer::run` redirects fd 1).
- A new scene template: `templates/scenes/<name>/{about.txt,scene/,project/}`
  and an entry in `tests/e2e/templates.sh`. Behaviors live in
  `templates/behaviors/`.
- A new opcode touches several places at once: `vm/Program.hpp` (Op),
  `vm/Parser.cpp` (syntax + the `Instructions` list), `vm/Vm.cpp`
  (execution), `boot/ProjectCheck.cpp` (reference checks), `docs/opcodes.md`
  and parser and VM tests.
- The project template (`templates/project/`) and `docs/` are embedded in the
  binary by `cmake/EmbedTemplate.cmake`. `{{NAME}}` becomes the project name.
- Changing the Lua API (`script/LuaRuntime.cpp`) means updating `docs/lua-api.md`.
- GUI (`gui/Gui.hpp`): data and `GuiSystem` (show by name, hover, clicks)
  without raylib; loading in `boot/XmlLoader.cpp` (`parse_gui_xml`), drawing
  in `render/Renderer.cpp` (`draw_gui`). A new GUI element: `ElementKind` +
  attributes in `parse_gui_xml` + drawing, `docs/gui.md` and tests in
  `tests/test_gui.cpp`.
