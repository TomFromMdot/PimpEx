# Lua scripts

Scripts live in `scripts/` (`*.lua` files). The engine loads all of them at
start, and reloads a file while the game runs after you save it.

Lua functions are called from `scene.swap` with `CALL`:

```
PROC collecting
  IF TOUCH Player Coin : CALL coin_collected
  IF KEY SPACE : CALL shoot Player 10
END
```

```lua
function shoot(who, speed)      -- arguments from CALL: "Player", 10
  local name = spawn("Bullet", nil, get(who, "x"), get(who, "y"))
  set(name, "vy", -speed)
end
```

`CALL` arguments that look like numbers reach Lua as numbers. The rest come
as text.

## Rules

- At the top level of a file only **define functions and variables**.
  `pimpex check` runs scripts in an empty environment, without the engine API.
- All scripts share one Lua state. Functions from `a.lua` are visible in `b.lua`.
- Globals survive script reloads and scene changes. Writing
  `list = list or {}` keeps the data after the file is saved.
- An error in a script does not stop the game. It goes to the console and to
  `.pimpex/runtime.log` as `scripts/file.lua:line: error: Lua: ...`.

## API

### Actors

| Function                              | Returns / does                                               |
|---------------------------------------|--------------------------------------------------------------|
| `get(actor, field)`                   | a number, text, or `nil` when the actor does not exist; fields as in `.swap`: `"x"`, `"Health.hp"`; also `get("Mouse", "dx")` |
| `set(actor, field, value)`            | sets a number, text or true/false; a missing component is added; errors raise a Lua error |
| `move(actor, dx, dy, [dz])`           | moves the actor                                              |
| `exists(actor)`                       | `true` if the actor exists                                   |
| `touching(a, b)`                      | `true` if the actors touch                                   |
| `spawn(archetype, [name], [x], [y], [z])` | the new actor's name, or `nil` without such an archetype |
| `destroy(actor)`                      | removes the actor                                            |
| `text(actor, "text {field}")`         | sets the actor's text                                        |
| `actors([selector])`                  | a table of actor names: all, with a component (`"Health"`) or with a tag (`"@enemy"`) |
| `has(actor, component)`               | `true` if the actor has the component                        |
| `add(actor, component)`               | adds a component with default values                         |
| `remove(actor, component)`            | removes a component                                          |
| `has_tag(actor, tag)`                 | `true` if the actor has the tag                              |
| `system(selector, function)`          | registers a system: `function(e)` every frame for each matching actor (docs/behaviors.md) |
| `raycast(x, y, z, dx, dy, dz, max, [ignore])` | a ray: the name of the first actor with a solid `Collider` and its distance (or `nil`); `ignore` – e.g. the shooter |
| `distance(a, b)`                      | the distance between the actors' centres                     |
| `is3d()`                              | `true` in a `mode="3d"` scene                                |

Fields are the same as in `.swap`: shortcuts (`x`, `y`, `z`, `vx`,
`rotation`…), `Component.field` (docs/components.md) and your own variables.
Actor behaviors (`<Behavior>`) are described in docs/behaviors.md.

### Input and time

| Function         | Returns                                          |
|------------------|--------------------------------------------------|
| `key("W")`       | the key is held                                  |
| `pressed("SPACE")` | the key went down this frame                   |
| `frame()`        | the current frame number                         |

### Engine

| Function                 | Does                                                              |
|--------------------------|-------------------------------------------------------------------|
| `log(...)`               | prints values to the console and `.pimpex/runtime.log`            |
| `scene("menu")`          | switches the scene after this frame                               |
| `swap.add("name")`       | pastes a PROC from `library.swap` into `scene.swap`               |
| `swap.remove("name")`    | removes a PROC from `scene.swap`                                  |
| `swap.paste("PROC ... END")` | pastes any `.swap` code (replaces a PROC with the same name)  |
| `swap.has("name")`       | `true` if the PROC is in `scene.swap` right now                   |
| `gui.show("pause")`      | shows the GUI scene from `gui/pause.xml` (docs/gui.md)            |
| `gui.hide("pause")`      | hides the GUI scene                                               |
| `gui.toggle("pause")`    | shows or hides it; returns `true` when the GUI is now visible     |
| `gui.shown("pause")`     | `true` if the GUI is visible                                      |

`swap.*` and `scene()` take effect after the frame ends; Boot then writes the
changed `scene.swap` to disk, so you see it in your editor. `gui.*` works at
once, and an unknown GUI name raises a Lua error.

```lua
-- Lua writes a procedure for the scene: an enemy chases the player for 5 seconds
function enrage(enemy)
  swap.paste(string.format([[
PROC rage_%s
  FOLLOW %s Player 3
  IF Scene.frame > %d : PROC_REMOVE rage_%s
END
]], enemy, enemy, frame() + 300, enemy))
end
```
