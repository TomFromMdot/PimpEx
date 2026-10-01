# Project files: `pimpex.xml` and `scene.xml`

## Project layout

```
my_game/
  pimpex.xml                 game settings: window, start scene
  scenes/<scene>/scene.xml   the scene's actors (who is on the board, how they look)
  scenes/<scene>/scene.swap  the scene's procedures (what happens) – docs/opcodes.md
  scenes/<scene>/library.swap  procedures pasted on demand (optional)
  components.xml             your own components (optional) – docs/components.md
  gui/<name>.xml             GUI scenes: menus, pause, buttons – docs/gui.md
  scripts/*.lua              Lua functions for CALL and systems – docs/lua-api.md
  scripts/behaviors/*.lua    actor behaviors – docs/behaviors.md
  assets/                    images
  .pimpex/runtime.log        errors and LOG output of the running game
  .pimpex/state.txt          a dump of the scene state (actors and their fields)
```

## `pimpex.xml`

```xml
<Project name="my_game">
  <Window title="My game" width="800" height="600" fps="60" background="#101018"/>
  <StartScene name="main"/>
  <HotReload keepState="true"/>
</Project>
```

- `StartScene` is a directory name in `scenes/`.
- `HotReload keepState="true"` (the default) means that after `scene.xml` is
  saved the actors stay where they are and only their looks change. With
  `false` the scene starts over.

## `scene.xml`

An actor is a name plus components (the full list is in docs/components.md).
`pimpex scene new <name> --template <template>` creates a scene from a
ready-made template (list: `pimpex scene list`).

```xml
<Scene background="#101018">
  <Resources>
    <Texture id="hero" path="assets/hero.png"/>
  </Resources>

  <Actor name="Player" x="400" y="300" w="40" h="40">
    <Rect color="blue"/>
    <Var name="speed" value="4"/>
  </Actor>

  <Actor name="Hero" x="100" y="100" w="64" h="64">
    <Sprite texture="hero"/>
  </Actor>

  <Actor name="Score" x="20" y="20" w="0" h="0" space="screen">
    <Text value="Points: {Game.points}" size="24" color="white"/>
  </Actor>

  <Archetype name="Bullet" w="6" h="12" vy="-8">
    <Rect color="yellow"/>
  </Archetype>
</Scene>
```

### `<Scene>` attributes

| Attribute    | Meaning                                                          |
|--------------|------------------------------------------------------------------|
| `background` | the scene's background color (instead of the one in `pimpex.xml`) |
| `mode`       | `2d` (default) or `3d`                                           |
| `gui`        | GUI scenes shown when the scene starts, e.g. `gui="hud"` or `gui="hud menu"` (files `gui/<name>.xml`, docs/gui.md) |

### `<Actor>` and `<Archetype>` attributes

| Attribute  | Default   | Meaning                                     |
|------------|-----------|---------------------------------------------|
| `name`     | required  | a unique name (no spaces, `.`, `/`, `:`, `@`) |
| `tags`     | –         | comma-separated tags, e.g. `enemy,flying` – for `@enemy` in `.swap` and Lua |
| `x`, `y`, `z` | 0      | the actor's **centre** (`z` – in 3D)        |
| `w`, `h`, `d` | 32 (1 in 3D) | width, height, depth (also the collision box) |
| `vx`, `vy`, `vz` | 0   | velocity per frame (adds a Body component)  |
| `gravity`  | 0         | gravity (Body)                              |
| `rotation` / `rz` | 0  | rotation in degrees (also `rx`, `ry` in 3D) |
| `scale`    | 1         | 3D model scale                              |
| `layer`    | 0         | 2D drawing order (higher = on top)          |
| `space`    | world     | `screen` = HUD: a position in screen pixels, independent of the camera |
| `visible`  | true      | whether the actor is drawn                  |

### Components (elements inside an actor)

The most common ones are below; all of them (including 3D, `Camera`, `Light`,
`Collider`) are in docs/components.md.

| Element                                    | Meaning                                         |
|--------------------------------------------|-------------------------------------------------|
| `<Rect color="..."/>`                      | a `w` × `h` rectangle                           |
| `<Circle color="..."/>`                    | a circle with diameter `w`                      |
| `<Sprite texture="id" tint="..."/>`        | an image from `<Resources>` stretched to `w`×`h` |
| `<Text value="..." size="20" color="..."/>`| text; `{Actor.field}` inserts a live value      |
| `<Var name="..." value="0"/>`              | an actor variable, e.g. lives, points           |
| `<Collider/>`                              | collisions: solid (a wall) or `kind="trigger"`  |
| `<Behavior name="..." .../>`               | a Lua behavior (docs/behaviors.md)              |

An **archetype** is a template for an actor that does not exist from the
start. `SPAWN` in `scene.swap` or `spawn()` in Lua creates it.

### Colors

`#rgb`, `#rrggbb`, `#rrggbbaa` or names: `white`, `black`, `red`, `green`,
`blue`, `yellow`, `orange`, `purple`, `pink`, `gray`, `brown`, `cyan`, `gold`,
`transparent`. Polish names work too: `biały`, `czarny`, `czerwony`,
`zielony`, `niebieski`, `żółty`, `pomarańczowy`, `fioletowy`, `różowy`,
`szary`, `brązowy`, `złoty`.

## ASCII maps: `<Tilemap>`

A level made of cells (walls, doors, enemies) can be drawn with characters.
Every map character is an actor from an archetype or the position of an
existing actor:

```xml
<Archetype name="Wall" y="1" w="2" h="2" d="2"><Cube color="gray"/><Collider/></Archetype>
<Archetype name="Coin" y="0.5" tags="coin"><Sphere color="gold"/></Archetype>
<Actor name="Player" y="0.6"><Collider/></Actor>

<Tilemap cell="2">
  <Legend char="#" archetype="Wall"/>
  <Legend char="$" archetype="Coin"/>
  <Legend char="P" actor="Player"/>
  <Map>
    ##########
    #P....$..#
    ##########
  </Map>
</Tilemap>
```

- `cell` is the cell size (default 1 in 3D, 32 in 2D). `x`, `y`, `z` offset
  the whole map.
- In 3D the map lies on the floor: a column is the `x` axis, a row the `z`
  axis, and `y` comes from the archetype. In 2D a column is `x`, a row `y`.
- `.` and spaces are empty cells. Common indentation of the rows is ignored.
- Map actors are named `Archetype_column_row`, e.g. `Wall_3_5`, and get the
  archetype's tags, so `@tag` handles them easily.
- Tiles without movement or behaviors (e.g. walls) are only counted in
  `state.txt` and the API, so hundreds of walls do not hide the rest.

## 3D scenes

```xml
<Scene mode="3d" background="#87ceeb">
  <Resources>
    <Model id="tree" path="assets/tree.glb"/>   <!-- .glb, .gltf, .obj -->
  </Resources>
  <Actor name="Ground" y="-0.5" w="30" h="1" d="30"><Cube color="green"/><Collider/></Actor>
  <Actor name="Tree" x="3" scale="1.5"><Model model="tree"/></Actor>
  <Actor name="Sun" x="5" y="10" z="5"><Light kind="directional" color="white"/></Actor>
  <Actor name="Cam"><Camera mode="orbit" target="Tree" distance="8" height="4"/></Actor>
</Scene>
```

- In 3D `y` grows **up** and units are "metres" (default size 1).
- Shapes: `Cube`, `Sphere`, `Cylinder`, `Plane`, `Model`.
- Cameras: `fixed` (from the camera actor's position), `follow` (behind the
  target), `orbit` (circles the target), `fps` (the target's eyes). Without a
  camera the scene slowly turns around its centre.
- With an `fps` camera the cursor starts free; a click in the window grabs it
  for mouse look, and ESC, switching windows or a GUI with buttons frees it.
- Without any `<Light>` a default sun shines. `kind="ambient"` sets the
  ambient brightness (default 0.35), and `directional` shines from its
  position towards (0, 0, 0).
- Text (`Text`) on 3D actors is drawn above them; `space="screen"` is the HUD.
