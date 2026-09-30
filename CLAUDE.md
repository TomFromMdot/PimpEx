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
- Nowy opcode wymaga zmian w wielu miejscach naraz: `vm/Program.hpp`
  (Op), `vm/Parser.cpp` (składnia + lista `Instructions`), `vm/Vm.cpp`
  (wykonanie), `boot/ProjectCheck.cpp` (sprawdzanie odwołań),
  `docs/opcodes.md` oraz testy parsera i VM.
- Szablon projektu (`templates/project/`) i `docs/` są wbudowane w binarkę
  przez `cmake/EmbedTemplate.cmake`. `{{NAME}}` zamienia się na nazwę projektu.
- Zmiana API Lua (`script/LuaRuntime.cpp`) wymaga aktualizacji `docs/lua-api.md`.
