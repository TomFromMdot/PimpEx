# Zachowania i systemy

Zachowania dopisuje się do aktorów bez kompilowania silnika. Są trzy sposoby,
od najprostszego:

| Sposób                           | Gdzie                         | Kiedy użyć                                   |
|----------------------------------|-------------------------------|----------------------------------------------|
| `PROC nazwa FOR @tag`            | `scene.swap`                  | proste reguły dla grupy aktorów              |
| `<Behavior name="...">`          | `scripts/behaviors/*.lua`     | zachowanie jednego aktora z własnym stanem   |
| `system("Komponent", funkcja)`   | `scripts/*.lua`               | logika dla wszystkich aktorów z komponentem  |

## 1. Systemy w `.swap`: `PROC ... FOR`

```
# Każda moneta (tag coin) sprawdza, czy dotyka gracza.
PROC monety FOR @coin
  IF TOUCH SELF Player : ADD Game.coins 1
  IF TOUCH SELF Player : DESTROY SELF
END

# Każdy aktor z komponentem Health: śmierć przy hp <= 0.
PROC smierc FOR Health
  IF SELF.Health.hp <= 0 : DESTROY SELF
END
```

`SELF` to aktor, dla którego właśnie wykonuje się ciało. `TOUCH SELF @enemy`
ustawia `OTHER` na dotkniętego wroga (docs/opcodes.md).

## 2. Zachowania Lua: `<Behavior>`

```xml
<Actor name="Ghost" x="400" y="300" tags="enemy">
  <Circle color="pink"/>
  <Behavior name="chase" target="Player" speed="1.5"/>
</Actor>
```

Plik `scripts/behaviors/chase.lua` **zwraca tabelę** z funkcjami, które
silnik woła sam:

```lua
return {
  on_start   = function(self) end,          -- raz, gdy aktor się pojawia
  on_update  = function(self) end,          -- co klatkę
  on_touch   = function(self, other) end,   -- styk z innym aktorem (oba muszą mieć <Collider>)
  on_destroy = function(self) end,          -- gdy aktor znika
}
```

### `self`

- Parametry z XML trafiają do `self`: `speed="1.5"` daje `self.speed == 1.5`.
  Liczby są liczbami, `true`/`false` to wartości logiczne.
- `self.name` to nazwa aktora.
- Pola aktora czyta się i zapisuje bezpośrednio: `self.x`, `self.y`, `self.z`,
  `self.w`, `self.h`, `self.d`, `self.rx`, `self.ry`, `self.rz`,
  `self.rotation`, `self.scale`, `self.layer`, `self.visible`, `self.vx`,
  `self.vy`, `self.vz`, `self.gravity`, `self.grounded`:

  ```lua
  self.x = self.x + 2
  ```
- Inne klucze (`self.timer`, `self.dir`) to zwykłe dane Lua, które
  zostają między klatkami.
- Metody: `self:get("Health.hp")`, `self:set("Rect.color", "red")`,
  `self:move(dx, dy, dz)`, `self:destroy()`, `self:exists()`,
  `self:has("Collider")`, `self:has_tag("enemy")`, `self:touching("Wall")`,
  `self:text("Napis")`, `self:distance("Player")`.
- Wzrok (promienie, zatrzymują je aktorzy z solidnym `Collider`):
  - `self:forward()` – kierunek patrzenia (w 3D z `ry`, 0 = w stronę -z;
    w 2D z `rotation`, 0 = w prawo);
  - `self:look(max)` – co jest przed aktorem: nazwa i odległość albo `nil`
    (np. trafienie przy strzale);
  - `self:sees("Player", max)` – czy nic solidnego nie zasłania celu;
  - `self:face("Player")` – obraca aktora przodem do celu.

Aktor może mieć kilka `<Behavior>`. Po zapisaniu pliku zachowania silnik
podmienia funkcje, a `self` (np. `self.dir`) zostaje. `on_start` nie
wykonuje się drugi raz.

### Gotowe zachowania

Każdy projekt dostaje je w `scripts/behaviors/`. Możesz je kopiować
i zmieniać.

| Plik              | Co robi                                         | Parametry                                   |
|-------------------|-------------------------------------------------|---------------------------------------------|
| `topdown_move`    | ruch strzałkami/WASD w 4 kierunkach             | `speed`                                     |
| `platformer_move` | bieg w bok i skok (wymaga `gravity` i Collider) | `speed`, `jump`                             |
| `chase`           | goni aktora                                     | `target`, `speed`, `axes` (`xy`/`xz`), `range` |
| `patrol`          | tam i z powrotem                                | `axis`, `distance`, `speed`                 |
| `spin`            | kręci się                                       | `axis` (`ry`/`rz`), `speed`                 |
| `move3d`          | 3D zza pleców: W/S przód, A/D obrót, skok       | `speed`, `turn`, `jump`                     |
| `fps_controller`  | 3D z oczu: mysz + WASD (z `<Camera mode="fps">`)| `speed`, `sensitivity`, `jump`, `camera`    |

## 3. Systemy Lua: `system()`

W dowolnym pliku `scripts/*.lua`, na górnym poziomie:

```lua
-- Co klatkę dla każdego aktora z komponentem Health.
system("Health", function(e)
  if e:get("Health.hp") <= 0 then e:destroy() end
end)

-- Dla każdego aktora z tagiem enemy.
system("@enemy", function(e)
  e.rotation = e.rotation + 1
end)
```

`e` działa jak `self` w zachowaniach, ale bez własnych danych (co klatkę
jest tworzony od nowa).

## Kolejność w jednej klatce

1. procedury z `scene.swap` (w tym `FOR`),
2. zachowania: `on_start` dla nowych, `on_destroy` dla usuniętych, potem `on_update`,
3. systemy Lua,
4. fizyka (`Body`, `Collider`) i `on_touch`,
5. zmiany zamówione w tej klatce: `PROC_ADD`, `PROC_REMOVE`, `swap.*`, `SCENE`.
