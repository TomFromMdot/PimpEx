# Behaviors and systems

Behavior is added to actors without compiling the engine. There are three
ways, from the simplest:

| Way                              | Where                         | When to use                                  |
|----------------------------------|-------------------------------|----------------------------------------------|
| `PROC name FOR @tag`             | `scene.swap`                  | simple rules for a group of actors           |
| `<Behavior name="...">`          | `scripts/behaviors/*.lua`     | one actor's behavior with its own state      |
| `system("Component", function)`  | `scripts/*.lua`               | logic for every actor with a component       |

## 1. Systems in `.swap`: `PROC ... FOR`

```
# Every coin (tag coin) checks whether it touches the player.
PROC coins FOR @coin
  IF TOUCH SELF Player : ADD Game.coins 1
  IF TOUCH SELF Player : DESTROY SELF
END

# Every actor with the Health component: dies at hp <= 0.
PROC death FOR Health
  IF SELF.Health.hp <= 0 : DESTROY SELF
END
```

`SELF` is the actor the body is running for. `TOUCH SELF @enemy` sets `OTHER`
to the enemy that was touched (docs/opcodes.md).

## 2. Lua behaviors: `<Behavior>`

```xml
<Actor name="Ghost" x="400" y="300" tags="enemy">
  <Circle color="pink"/>
  <Behavior name="chase" target="Player" speed="1.5"/>
</Actor>
```

The file `scripts/behaviors/chase.lua` **returns a table** of functions the
engine calls by itself:

```lua
return {
  on_start   = function(self) end,          -- once, when the actor appears
  on_update  = function(self) end,          -- every frame
  on_touch   = function(self, other) end,   -- contact with another actor (both need a <Collider>)
  on_destroy = function(self) end,          -- when the actor goes away
}
```

### `self`

- XML parameters end up in `self`: `speed="1.5"` gives `self.speed == 1.5`.
  Numbers are numbers, `true`/`false` are booleans.
- `self.name` is the actor's name.
- Actor fields are read and written directly: `self.x`, `self.y`, `self.z`,
  `self.w`, `self.h`, `self.d`, `self.rx`, `self.ry`, `self.rz`,
  `self.rotation`, `self.scale`, `self.layer`, `self.visible`, `self.vx`,
  `self.vy`, `self.vz`, `self.gravity`, `self.grounded`:

  ```lua
  self.x = self.x + 2
  ```
- Other keys (`self.timer`, `self.dir`) are plain Lua data kept between frames.
- Methods: `self:get("Health.hp")`, `self:set("Rect.color", "red")`,
  `self:move(dx, dy, dz)`, `self:destroy()`, `self:exists()`,
  `self:has("Collider")`, `self:has_tag("enemy")`, `self:touching("Wall")`,
  `self:text("Hello")`, `self:distance("Player")`.
- Sight (rays, stopped by actors with a solid `Collider`):
  - `self:forward()` – the facing direction (in 3D from `ry`, 0 = towards -z;
    in 2D from `rotation`, 0 = right);
  - `self:look(max)` – what is in front of the actor: a name and distance or
    `nil` (e.g. what a shot hits);
  - `self:sees("Player", max)` – nothing solid blocks the view of the target;
  - `self:face("Player")` – turns the actor to face the target.

An actor can have several `<Behavior>`s. When a behavior file is saved the
engine swaps the functions and keeps `self` (e.g. `self.dir`). `on_start`
does not run again.

### Ready-made behaviors

Every project gets these in `scripts/behaviors/`. Copy and change them freely.

| File              | What it does                                    | Parameters                                  |
|-------------------|-------------------------------------------------|---------------------------------------------|
| `topdown_move`    | arrows/WASD movement in 4 directions            | `speed`                                     |
| `platformer_move` | running and jumping (needs `gravity` and a Collider) | `speed`, `jump`                        |
| `chase`           | chases an actor                                 | `target`, `speed`, `axes` (`xy`/`xz`), `range` |
| `patrol`          | back and forth                                  | `axis`, `distance`, `speed`                 |
| `spin`            | spins                                           | `axis` (`ry`/`rz`), `speed`                 |
| `move3d`          | 3D third person: W/S forward, A/D turn, jump    | `speed`, `turn`, `jump`                     |
| `fps_controller`  | 3D first person: mouse + WASD (with `<Camera mode="fps">`) | `speed`, `sensitivity`, `jump`, `camera` |

## 3. Lua systems: `system()`

In any `scripts/*.lua` file, at the top level:

```lua
-- Every frame for each actor with the Health component.
system("Health", function(e)
  if e:get("Health.hp") <= 0 then e:destroy() end
end)

-- For each actor with the enemy tag.
system("@enemy", function(e)
  e.rotation = e.rotation + 1
end)
```

`e` works like `self` in behaviors but has no data of its own (it is created
anew every frame).

## Order within one frame

1. GUI clicks and button keys (docs/gui.md),
2. procedures from `scene.swap` (including `FOR`),
3. behaviors: `on_start` for new actors, `on_destroy` for removed ones, then `on_update`,
4. Lua systems,
5. physics (`Body`, `Collider`) and `on_touch`,
6. changes requested during the frame: `PROC_ADD`, `PROC_REMOVE`, `swap.*`, `SCENE`.

While a GUI with `pause="true"` is visible, steps 2–5 are skipped.
