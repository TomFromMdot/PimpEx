# Pliki projektu: `pimpex.xml` i `scene.xml`

## Układ projektu

```
moja_gra/
  pimpex.xml                 ustawienia gry: okno, scena startowa
  scenes/<scena>/scene.xml   aktorzy sceny (kto jest na planszy, jak wygląda)
  scenes/<scena>/scene.swap  procedury sceny (co się dzieje) – docs/opcodes.md
  scenes/<scena>/library.swap  procedury do wklejania na żądanie (opcjonalnie)
  components.xml             własne komponenty (opcjonalnie) – docs/components.md
  scripts/*.lua              funkcje Lua wołane przez CALL i systemy – docs/lua-api.md
  scripts/behaviors/*.lua    zachowania aktorów – docs/behaviors.md
  assets/                    obrazki
  .pimpex/runtime.log        błędy i LOG z działającej gry
  .pimpex/state.txt          zrzut stanu sceny (aktorzy i ich pola)
```

## `pimpex.xml`

```xml
<Project name="moja_gra">
  <Window title="Moja gra" width="800" height="600" fps="60" background="#101018"/>
  <StartScene name="main"/>
  <HotReload keepState="true"/>
</Project>
```

- `StartScene` to nazwa katalogu w `scenes/`.
- `HotReload keepState="true"` (domyślnie) oznacza, że po zapisaniu
  `scene.xml` aktorzy zostają tam, gdzie są, a zmienia się tylko ich wygląd.
  Przy `false` scena startuje od nowa.

## `scene.xml`

Aktor to nazwa + komponenty (pełna lista w docs/components.md). Nową scenę
z gotowego szablonu tworzy `pimpex scene new <nazwa> --template <szablon>`
(lista: `pimpex scene list`).

```xml
<Scene background="#101018">
  <Resources>
    <Texture id="hero" path="assets/hero.png"/>
  </Resources>

  <Actor name="Player" x="400" y="300" w="40" h="40">
    <Rect color="niebieski"/>
    <Var name="speed" value="4"/>
  </Actor>

  <Actor name="Hero" x="100" y="100" w="64" h="64">
    <Sprite texture="hero"/>
  </Actor>

  <Actor name="Score" x="20" y="20" w="0" h="0" space="screen">
    <Text value="Punkty: {Game.points}" size="24" color="white"/>
  </Actor>

  <Archetype name="Bullet" w="6" h="12" vy="-8">
    <Rect color="yellow"/>
  </Archetype>
</Scene>
```

### Atrybuty `<Actor>` i `<Archetype>`

| Atrybut    | Domyślnie | Znaczenie                                   |
|------------|-----------|---------------------------------------------|
| `name`     | wymagany  | unikalna nazwa (bez spacji, `.`, `/`, `:`, `@`) |
| `tags`     | –         | tagi po przecinku, np. `enemy,flying` – do `@enemy` w `.swap` i Lua |
| `x`, `y`, `z` | 0      | pozycja **środka** aktora (`z` – w 3D)      |
| `w`, `h`, `d` | 32 (w 3D: 1) | szerokość, wysokość, głębokość (też kształt kolizji) |
| `vx`, `vy`, `vz` | 0   | prędkość na klatkę (dodaje komponent Body)  |
| `gravity`  | 0         | grawitacja (Body)                           |
| `rotation` / `rz` | 0  | obrót w stopniach (w 3D też `rx`, `ry`)     |
| `scale`    | 1         | skala modelu 3D                             |
| `layer`    | 0         | kolejność rysowania 2D (większe = wyżej)    |
| `space`    | world     | `screen` = HUD: pozycja w pikselach ekranu, niezależna od kamery |
| `visible`  | true      | czy aktor jest rysowany                     |

### Komponenty (elementy w środku aktora)

Najczęstsze poniżej, a wszystkie (także 3D, `Camera`, `Light`, `Collider`)
w docs/components.md.

| Element                                    | Znaczenie                                       |
|--------------------------------------------|-------------------------------------------------|
| `<Rect color="..."/>`                      | prostokąt `w` × `h`                             |
| `<Circle color="..."/>`                    | koło o średnicy `w`                             |
| `<Sprite texture="id" tint="..."/>`        | obrazek z `<Resources>` rozciągnięty do `w`×`h` |
| `<Text value="..." size="20" color="..."/>`| napis; `{Aktor.pole}` wstawia wartość na żywo   |
| `<Var name="..." value="0"/>`              | zmienna aktora, np. życia, punkty               |
| `<Collider/>`                              | kolizje: solid (ściana) albo `kind="trigger"`   |
| `<Behavior name="..." .../>`               | zachowanie Lua (docs/behaviors.md)              |

**Archetyp** to wzór aktora, który nie istnieje od początku. Tworzy go
`SPAWN` w `scene.swap` albo `spawn()` w Lua.

### Kolory

`#rgb`, `#rrggbb`, `#rrggbbaa` albo nazwy: `white`, `black`, `red`, `green`,
`blue`, `yellow`, `orange`, `purple`, `pink`, `gray`, `brown`, `cyan`, `gold`,
`transparent`. Po polsku: `biały`, `czarny`, `czerwony`, `zielony`,
`niebieski`, `żółty`, `pomarańczowy`, `fioletowy`, `różowy`, `szary`,
`brązowy`, `złoty`.

## Sceny 3D

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

- W 3D `y` rośnie **w górę**, a jednostki to „metry” (domyślny rozmiar 1).
- Bryły: `Cube`, `Sphere`, `Cylinder`, `Plane`, `Model`.
- Kamera: `fixed` (z pozycji aktora-kamery), `follow` (za plecami celu),
  `orbit` (krąży wokół celu), `fps` (oczy celu). Bez kamery scena powoli
  obraca się wokół środka.
- Bez żadnego `<Light>` świeci domyślne „słońce”.
- Napisy (`Text`) na aktorach 3D rysują się nad nimi, a `space="screen"` to HUD.
