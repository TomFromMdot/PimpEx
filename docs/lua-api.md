# Skrypty Lua

Skrypty leżą w katalogu `scripts/` (pliki `*.lua`). Silnik wczytuje je
wszystkie na starcie, a po zapisaniu pliku wczytuje go ponownie w trakcie gry.

Funkcje Lua wywołuje się z `scene.swap` instrukcją `CALL`:

```
PROC zbieranie
  IF TOUCH Player Coin : CALL coin_collected
  IF KEY SPACE : CALL shoot Player 10
END
```

```lua
function shoot(who, speed)      -- argumenty z CALL: "Player", 10
  local name = spawn("Bullet", nil, get(who, "x"), get(who, "y"))
  set(name, "vy", -speed)
end
```

Argumenty z `CALL`, które wyglądają jak liczby, trafiają do Lua jako liczby.
Pozostałe przychodzą jako tekst.

## Zasady

- Na górnym poziomie pliku tylko **definiuj funkcje i zmienne**.
  `pimpex check` wykonuje skrypty w pustym środowisku, bez API silnika.
- Wszystkie skrypty dzielą jeden stan Lua. Funkcje z `a.lua` widać w `b.lua`.
- Zmienne globalne przetrwają przeładowanie skryptu i zmianę sceny. Zapis
  `lista = lista or {}` zachowuje dane po zapisaniu pliku.
- Błąd w skrypcie nie zatrzymuje gry. Trafia do konsoli i do
  `.pimpex/runtime.log` jako `scripts/plik.lua:linia: błąd: Lua: ...`.

## API

### Aktorzy

| Funkcja                               | Zwraca / robi                                                |
|---------------------------------------|--------------------------------------------------------------|
| `get(aktor, pole)`                    | liczba, tekst albo `nil`, gdy aktor nie istnieje; pole jak w `.swap`: `"x"`, `"Health.hp"`; też `get("Mouse", "dx")` |
| `set(aktor, pole, wartość)`           | ustawia liczbę, tekst albo true/false; brakujący komponent zostaje dodany; błąd = wyjątek Lua |
| `move(aktor, dx, dy, [dz])`           | przesuwa aktora                                              |
| `exists(aktor)`                       | `true`, jeśli aktor istnieje                                 |
| `touching(a, b)`                      | `true`, jeśli aktorzy się stykają                            |
| `spawn(archetyp, [nazwa], [x], [y], [z])` | nazwa nowego aktora albo `nil`, gdy nie ma archetypu     |
| `destroy(aktor)`                      | usuwa aktora                                                 |
| `text(aktor, "tekst {pole}")`         | ustawia napis aktora                                         |
| `actors([selektor])`                  | tabela nazw aktorów: wszystkich, z komponentem (`"Health"`) albo z tagiem (`"@enemy"`) |
| `has(aktor, komponent)`               | `true`, jeśli aktor ma komponent                             |
| `add(aktor, komponent)`               | dodaje komponent z wartościami domyślnymi                    |
| `remove(aktor, komponent)`            | usuwa komponent                                              |
| `has_tag(aktor, tag)`                 | `true`, jeśli aktor ma tag                                   |
| `system(selektor, funkcja)`           | rejestruje system: `funkcja(e)` co klatkę dla każdego pasującego aktora (docs/behaviors.md) |
| `raycast(x, y, z, dx, dy, dz, max, [pomiń])` | promień: nazwa pierwszego aktora z solidnym `Collider` i odległość (albo `nil`); `pomiń` – np. strzelający |
| `distance(a, b)`                      | odległość między środkami aktorów                            |
| `is3d()`                              | `true` w scenie `mode="3d"`                                  |

Pola są takie same jak w `.swap`: skróty (`x`, `y`, `z`, `vx`, `rotation`…),
`Komponent.pole` (docs/components.md) i własne zmienne. Zachowania aktorów
(`<Behavior>`) opisuje docs/behaviors.md.

### Wejście i czas

| Funkcja          | Zwraca                                           |
|------------------|--------------------------------------------------|
| `key("W")`       | klawisz trzymany                                 |
| `pressed("SPACE")` | klawisz wciśnięty w tej klatce                 |
| `frame()`        | numer bieżącej klatki                            |

### Silnik

| Funkcja                  | Robi                                                              |
|--------------------------|-------------------------------------------------------------------|
| `log(...)`               | wypisuje wartości w konsoli i w `.pimpex/runtime.log`             |
| `scene("menu")`          | przełącza scenę po tej klatce                                     |
| `swap.add("nazwa")`      | wkleja PROC z `library.swap` do `scene.swap`                      |
| `swap.remove("nazwa")`   | usuwa PROC z `scene.swap`                                         |
| `swap.paste("PROC ... END")` | wkleja dowolny kod `.swap` (zastępuje PROC o tej samej nazwie) |
| `swap.has("nazwa")`      | `true`, jeśli PROC jest teraz w `scene.swap`                      |

`swap.*` i `scene()` działają po zakończeniu klatki. Boot zapisuje wtedy
zmieniony `scene.swap` na dysku, więc widać go w edytorze.

```lua
-- Lua pisze procedurę dla sceny: przeciwnik goni gracza przez 5 sekund
function enrage(enemy)
  swap.paste(string.format([[
PROC rage_%s
  FOLLOW %s Player 3
  IF Scene.frame > %d : PROC_REMOVE rage_%s
END
]], enemy, enemy, frame() + 300, enemy))
end
```
