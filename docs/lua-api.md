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
| `get(aktor, pole)`                    | liczba albo `nil`, gdy aktor nie istnieje; też `Screen.w`, `Mouse.x`, `Scene.frame` |
| `set(aktor, pole, wartość)`           | `true`/`false`; tworzy zmienną, jeśli jej nie było           |
| `move(aktor, dx, dy)`                 | przesuwa aktora                                              |
| `exists(aktor)`                       | `true`, jeśli aktor istnieje                                 |
| `touching(a, b)`                      | `true`, jeśli aktorzy się stykają                            |
| `spawn(archetyp, [nazwa], [x], [y])`  | nazwa nowego aktora albo `nil`, gdy nie ma archetypu         |
| `destroy(aktor)`                      | usuwa aktora                                                 |
| `text(aktor, "tekst {pole}")`         | ustawia napis aktora                                         |
| `actors([archetyp])`                  | tabela nazw wszystkich aktorów (albo tylko z archetypu)      |

Pola są takie same jak w `.swap`: `x`, `y`, `vx`, `vy`, `w`, `h`,
`rotation`, `z`, `visible` i własne zmienne.

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
