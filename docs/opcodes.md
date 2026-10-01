# The `.swap` language – scene procedures

`scene.swap` is the **scene loop program**. It is made of procedures (`PROC`).
The engine runs **every procedure every frame** (60 times a second), top to
bottom, for as long as the game runs. This is the scene's "endless process".

You can change the file **while the game runs**. After you save, the engine
loads the new version between frames, without a restart. If the file has an
error, the engine keeps running the previous valid version and writes the
error to `.pimpex/runtime.log`.

```
# This is a comment – the engine skips it.
PROC player_movement
  IF KEY LEFT  : MOVE Player -4 0
  IF KEY RIGHT : MOVE Player 4 0
END
```

## Procedures

| Syntax                | Meaning                                                      |
|-----------------------|--------------------------------------------------------------|
| `PROC name` … `END`   | runs every frame                                             |
| `PROC name EVERY 30`  | runs every 30 frames (60 frames = 1 second)                  |
| `PROC name ONCE`      | runs once, then the engine **removes it from the file**      |
| `PROC name FOR @tag`  | the body runs for **every** actor with the tag (`SELF`)      |
| `PROC name FOR Health` | … for every actor with the `Health` component               |

Modes can be combined, e.g. `PROC regeneration FOR Health EVERY 60`.

`ONCE` is handy for "pasting" one-off commands into a running game. Append a
`ONCE` procedure to the file, save, and the engine runs it and cleans up.

Procedure names must be unique in a file.

## Values

Wherever an instruction expects a number you can write:

- a number: `4`, `-2.5`
- an actor field: `Player.x`, `Game.score`, `Player.Health.hp`
- a negated field: `-Ball.vx`
- a random number: `RANDOM 1 6` (a number between 1 and 6, with fractions)

## Actor fields

| Field       | Meaning                                                |
|-------------|--------------------------------------------------------|
| `x`, `y`    | the actor's centre (pixels, `y` grows down)            |
| `vx`, `vy`  | velocity – how far the engine moves the actor each frame |
| `w`, `h`    | width and height                                       |
| `rotation`  | rotation in degrees                                    |
| `visible`   | `1` – visible, `0` – hidden                            |
| `z`, `vz`, `d`, `rx`, `ry` | the third axis, its velocity, depth and 3D rotation |
| `Component.field` | a component field, e.g. `Player.Health.hp`, `Cam.Camera.zoom` (docs/components.md) |
| other names | your variables, e.g. `Game.score`, `Player.lives`      |

A variable that does not exist yet is created by the first `SET` or `ADD`.
Reading a missing variable gives `0`. Variables hold numbers only. Text
component fields are set with quoted text:
`SET Hud.Text.value "Well done!"`, `SET Player.Rect.color "red"`.

### SELF and OTHER

- `SELF` – in `PROC ... FOR`, the actor the body is running for:
  `MOVE SELF 1 0`, `IF SELF.Health.hp <= 0 : DESTROY SELF`.
- `OTHER` – the actor found by the last `TOUCH` with a group:
  `IF TOUCH SELF @enemy : ADD OTHER.Health.hp -1`.

Special read-only fields:

- `Screen.w`, `Screen.h` – the window size
- `Mouse.x`, `Mouse.y` – the mouse position, `Mouse.dx`, `Mouse.dy` – its motion this frame
- `Mouse.left`, `Mouse.right` – `1` while the mouse button is held
- `Scene.frame` – the current frame number

## Instructions

| Instruction                             | What it does                                                   |
|-----------------------------------------|----------------------------------------------------------------|
| `MOVE actor dx dy [dz]`                 | moves the actor by `dx`, `dy` (and `dz` in 3D)                 |
| `SET Actor.field value`                 | sets a field (to a number or "quoted text")                    |
| `ADD Actor.field value`                 | adds to a field (a negative value subtracts)                   |
| `MUL Actor.field value`                 | multiplies a field (e.g. `MUL Ball.vx -1` bounces the ball)    |
| `SPAWN Archetype [AS name] [AT x y [z]]`| creates a new actor from an archetype in `scene.xml`           |
| `DESTROY actor`                         | removes the actor                                              |
| `FOLLOW actor target speed`             | moves the actor towards the target (also along z in 3D)        |
| `TEXT actor "text"`                     | changes the actor's text; `{Actor.field}` inserts a field value |
| `LOG "text"`                            | prints text to the console and `.pimpex/runtime.log`           |
| `CALL function [args…]`                 | calls a Lua function from the scripts in `scripts/`            |
| `PROC_ADD name`                         | pastes procedure `name` from `library.swap` into `scene.swap`  |
| `PROC_REMOVE name`                      | removes procedure `name` from `scene.swap`                     |
| `SCENE name`                            | switches to another scene (`scenes/name/`)                     |
| `GUI_SHOW name`                         | shows the GUI scene from `gui/name.xml` (menus, pause – docs/gui.md) |
| `GUI_HIDE name`                         | hides the GUI scene                                            |
| `GUI_TOGGLE name`                       | shows or hides the GUI scene (e.g. a pause on ESCAPE)          |

`SPAWN` without `AS` names the actor automatically (`Bullet_1`, `Bullet_2`…).
Without `AT` the new actor appears at the archetype's position.

In `TEXT`, `{field}` without an actor name means a field of the same actor,
e.g. `TEXT Score "Points: {points}"`.

## Conditions: `IF`

```
IF condition : instruction
IF condition AND condition : instruction
IF condition : IF other_condition : instruction
```

| Condition                 | True when…                                           |
|---------------------------|------------------------------------------------------|
| `KEY W`                   | the key is held                                      |
| `PRESSED SPACE`           | the key went down this frame (once)                  |
| `TOUCH a b`               | actors `a` and `b` touch (boxes w, h, d)             |
| `TOUCH a @tag`            | `a` touches **any** actor with the tag – it becomes `OTHER` |
| `TOUCH a Component`       | the same, for actors with the component              |
| `EXISTS actor`            | the actor exists                                     |
| `EXISTS @tag`             | at least one actor has the tag                       |
| `CHANCE 10`               | a 10% chance came up (rolled separately every frame) |
| `value < value`           | comparison: `<`, `<=`, `>`, `>=`, `==`, `!=`         |
| `NOT condition`           | negates the condition                                |

Keys: `A`–`Z`, `0`–`9`, `SPACE`, `ENTER`, `ESCAPE`, `SHIFT`,
`LEFT`, `RIGHT`, `UP`, `DOWN`.

## The procedure library: `library.swap`

`library.swap` may sit next to `scene.swap`. Library procedures **do not run
on their own**. They are ready-made blocks the engine (Boot) pastes into
`scene.swap` on request: with `PROC_ADD name` or from Lua with
`swap.add("name")`. `PROC_REMOVE` and `swap.remove("name")` cut a procedure
out of `scene.swap`.

```
# library.swap
PROC rain EVERY 20
  SPAWN Drop AT RANDOM 0 800 0
END
```

## Checking

`pimpex check <project>` checks every file without running the game and
prints errors as `file:line: error: message`.
