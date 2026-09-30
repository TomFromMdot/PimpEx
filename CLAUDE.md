# PimpEx – praca nad silnikiem

Silnik gier dla dzieci, w którym cała gra to tekst. Wizja i architektura są
w `README.md`, a formaty plików gry w `docs/`.

## Budowanie i testy

```
cmake --preset linux-debug          # vcpkg z $VCPKG_ROOT albo ~/vcpkg
cmake --build build
ctest --test-dir build --output-on-failure
```

- Testy jednostkowe (doctest) są w `tests/*.cpp`. VM, Boot i Lua testuje się
  bez okna (`vm::ManualInput`, `BootFixture` w `tests/test_boot.cpp`).
- Testy end-to-end w `CMakeLists.txt` sprawdzają szablon (`pimpex init`)
  i każdy katalog w `examples/` przez `pimpex check` i `play --headless`.
- Do podglądu na żywo służy `./build/pimpex play examples/pong --frames 60 --screenshot shot.png`.

## Zasady

- C++23, komentarze i komunikaty dla użytkownika **po polsku**. Błędy
  zawsze w formacie `plik:linia: błąd: opis` (`Diagnostic`).
- Tylko `render/` i `core/Engine.cpp` znają raylib. VM, Boot i World muszą
  działać bez okna.
- Tylko `boot::Boot` zapisuje `scene.swap`, zawsze przez `write_file_atomic`
  i po `expect()` na watcherze.
- Nowy komponent wbudowany: struktura w `scene/Components.hpp` + deskryptor
  (`Binder`) w `ComponentRegistry::with_builtins()`. Loader, `.swap`, Lua,
  API i dump działają same. Potem rysowanie (`render/`) albo logika
  (`scene/Physics.cpp`), regeneracja `docs/components.md`
  (`pimpex components --markdown > docs/components.md`, pilnuje tego ctest)
  i testy w `tests/test_components.cpp`.
- Nowa metoda API: `api/LiveApi.cpp` (`dispatch` + tabela `Methods`),
  `docs/api.md`, test w `tests/test_api.cpp`. Jeśli ma być dostępna dla
  agentów, także narzędzie w `api/McpServer.cpp` (`tools_list` +
  `call_tool`) i `tests/test_mcp.cpp`.
- Serwer MCP pisze protokół na stdout. Wszystko inne musi iść na stderr
  (`McpServer::run` przekierowuje fd 1).
- Nowy szablon sceny: `templates/scenes/<nazwa>/{about.txt,scene/,project/}`
  oraz wpis w `tests/e2e/templates.sh`. Zachowania trzymamy w
  `templates/behaviors/`.
- Nowy opcode wymaga zmian w wielu miejscach naraz: `vm/Program.hpp`
  (Op), `vm/Parser.cpp` (składnia + lista `Instructions`), `vm/Vm.cpp`
  (wykonanie), `boot/ProjectCheck.cpp` (sprawdzanie odwołań),
  `docs/opcodes.md` oraz testy parsera i VM.
- Szablon projektu (`templates/project/`) i `docs/` są wbudowane w binarkę
  przez `cmake/EmbedTemplate.cmake`. `{{NAME}}` zamienia się na nazwę projektu.
- Zmiana API Lua (`script/LuaRuntime.cpp`) wymaga aktualizacji `docs/lua-api.md`.
