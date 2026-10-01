# GUI – menus, pause, buttons

GUI screens with buttons and text are drawn **on top of the game**, in window
pixels. Every file `gui/<name>.xml` in the project directory is one **GUI
scene**, e.g. `gui/menu.xml`, `gui/pause.xml`, `gui/hud.xml`. You turn one on
by name:

| From                     | How                                                   |
|--------------------------|-------------------------------------------------------|
| `scene.swap`             | `GUI_SHOW pause`, `GUI_HIDE pause`, `GUI_TOGGLE pause` |
| a GUI button             | `click="GUI_HIDE pause; SCENE menu"`                  |
| Lua                      | `gui.show("pause")`, `gui.hide`, `gui.toggle`, `gui.shown` |
| `scene.xml`              | `<Scene gui="hud">` – GUIs shown when the scene starts |

GUI scenes belong to the whole project, not to one game scene. Several can be
visible at once (e.g. `hud` and `pause`); the last one shown is on top. Every
time a scene loads (`SCENE`), exactly the GUIs listed in its
`<Scene gui="...">` are shown and the rest are hidden.

```xml
<!-- gui/pause.xml -->
<Gui pause="true" dim="#00000099">
  <Panel  anchor="center" w="340" h="280" color="#1c1c2c" border="#ffffff40"/>
  <Label  anchor="center" y="-90" text="Paused" size="44" color="gold"/>
  <Button anchor="center" y="-10" text="Resume" key="ESCAPE" click="GUI_HIDE pause"/>
  <Button anchor="center" y="60"  text="Restart" click="GUI_HIDE pause; SCENE main"/>
</Gui>
```

```
# scene.swap
PROC pause_menu
  IF PRESSED ESCAPE : GUI_SHOW pause
END
```

## `<Gui>`

| Attribute | Meaning                                                                  |
|-----------|--------------------------------------------------------------------------|
| `pause`   | `true` – while this GUI is visible the game stops (procedures, Lua and physics). Clicks still work. |
| `dim`     | a color that dims the whole window below the GUI, e.g. `#00000099`       |

## Elements

Elements are drawn in file order (later ones on top).

| Element    | What it is                       | Its own attributes                                   |
|------------|----------------------------------|------------------------------------------------------|
| `<Panel>`  | a rectangle (a window background) | `color` (default: translucent black), `border`      |
| `<Label>`  | text                             | `text`, `size`, `color`, `align` (`left`, `center`, `right`) |
| `<Button>` | a button                         | `text`, `size`, `color`, `hover`, `text_color`, `border`, `click`, `key` |
| `<Image>`  | an image                         | `src` (a file relative to the project), `color` (tint) |

Common attributes:

| Attribute | Meaning                                                                  |
|-----------|--------------------------------------------------------------------------|
| `anchor`  | the window point the position is measured from: `top-left` (default), `top`, `top-right`, `left`, `center`, `right`, `bottom-left`, `bottom`, `bottom-right` |
| `x`, `y`  | the element's **centre** relative to the anchor (pixels, `y` grows down) |
| `w`, `h`  | width and height (buttons: 220 × 56 by default)                         |
| `name`    | the element's name (for descriptions only)                               |

The anchor keeps the layout when the window is resized: `anchor="center"`
stays in the middle and `anchor="bottom-right" x="-80" y="-30"` in the
bottom-right corner.

`text` can contain field values, just like `TEXT` in `.swap`:
`text="Points: {Game.score}"`.

## Buttons

- `click` – `.swap` instructions separated by semicolons, run on click:
  `click="SET Game.level 2; GUI_HIDE menu; SCENE level"`. Every instruction
  from docs/opcodes.md works except `PROC_ADD`, including `CALL` into Lua.
  No `SELF` or `OTHER`.
- `key` – a key that acts as a click, e.g. `key="ESCAPE"` on "Resume" or
  `key="ENTER"` on "Play".
- A click is pressing and releasing the left mouse button on the same button.
  Under the cursor the button gets the `hover` color.
- Panels and buttons cover what is below them. A GUI with `pause="true"`
  covers every GUI below it.

While a GUI with buttons is visible, an fps camera frees the cursor so you
can click.

## Headless tests

`--input` accepts clicks: `CLICK@x/y:frame` (or `CLICK@x/y:from-to`) puts the
cursor at `(x, y)` and holds the left mouse button:

```
pimpex play . --headless --frames 30 --input "CLICK@400/300:5"
```

The `gui:` line in `.pimpex/state.txt` lists the visible GUIs (with
`(paused)` while the game is stopped).

## Checking

`pimpex check` checks `gui/*.xml` files, the instructions in `click`, and GUI
names in `GUI_SHOW`/`GUI_HIDE`/`GUI_TOGGLE` and `<Scene gui>`. When a GUI file
is saved while the game runs (`pimpex dev`), the engine swaps it live.
