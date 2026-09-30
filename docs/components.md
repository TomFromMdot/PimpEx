# Komponenty

Aktor (GameObject) to nazwa + zestaw komponentów. Komponent to dane: jak aktor
wygląda, jak się porusza, co przechowuje. Zachowania dopisuje się w Lua
(`<Behavior>`, docs/behaviors.md) albo w `.swap` (`PROC ... FOR Komponent`).

```xml
<Actor name="Player" x="100" y="200" w="32" h="32" tags="hero">
  <Rect color="blue"/>
  <Collider/>
  <Health hp="5"/>                      <!-- komponent z components.xml -->
  <Behavior name="topdown_move" speed="4"/>
</Actor>
```

- Pola `Transform` i `Body` można pisać wprost na `<Actor>` (`x="10" vx="2"`).
- W `.swap` i Lua pole to `Aktor.pole` (skróty: `x`, `vx`, `rotation`...) albo
  `Aktor.Komponent.pole`, np. `Player.Health.hp`, `Cam.Camera.zoom`.
- Brakujący komponent zostaje dodany przy pierwszym `SET`.
- Ten plik powstaje z `pimpex components --markdown`. Dla projektu z
  `components.xml` ta komenda pokazuje też komponenty użytkownika.

## Komponenty użytkownika: `components.xml`

```xml
<Components>
  <Component name="Health" doc="Punkty życia">
    <Field name="hp" type="number" default="3"/>
    <Field name="shield" type="bool" default="false"/>
  </Component>
</Components>
```

Typy pól: `number`, `integer`, `bool`, `text`, `color`.

## Wbudowane: ogólne

### `<Transform>`

Pozycja, rozmiar, obrót i widoczność. Każdy aktor go ma; jego pola można pisać wprost na <Actor>.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `x` | liczba | `0` | pozycja X (środek) |
| `y` | liczba | `0` | pozycja Y (środek); w 2D rośnie w dół, w 3D w górę |
| `z` | liczba | `0` | pozycja Z (3D) |
| `w` | liczba | `32` | szerokość |
| `h` | liczba | `32` | wysokość |
| `d` | liczba | `32` | głębokość (3D) |
| `rx` | liczba | `0` | obrót wokół osi X w stopniach (3D) |
| `ry` | liczba | `0` | obrót wokół osi Y w stopniach (3D) |
| `rz` | liczba | `0` | obrót wokół osi Z w stopniach (w 2D: obrót) |
| `scale` | liczba | `1` | skala (3D, modele) |
| `layer` | liczba całkowita | `0` | kolejność rysowania 2D (większe = wyżej) |
| `visible` | true/false | `true` | czy aktor jest rysowany |
| `space` | world \| screen | `world` | world = w świecie, screen = HUD w pikselach ekranu |

## Wbudowane: ruch

### `<Body>`

Prędkość i grawitacja. Silnik przesuwa aktora co klatkę.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `vx` | liczba | `0` | prędkość X na klatkę |
| `vy` | liczba | `0` | prędkość Y na klatkę |
| `vz` | liczba | `0` | prędkość Z na klatkę (3D) |
| `gravity` | liczba | `0` | przyspieszenie w dół na klatkę (np. 0.5) |
| `grounded` | true/false | `false` | 1, gdy stoi na czymś solidnym (tylko do odczytu) |

### `<Collider>`

Kolizje z prostokątem/prostopadłościanem aktora (w, h, d).

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `kind` | solid \| trigger | `solid` | solid = nie da się przez niego przejść, trigger = tylko wykrywa dotyk |

## Wbudowane: 2D

### `<Rect>`

Prostokąt w × h.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `color` | kolor | `white` | kolor |

### `<Circle>`

Koło o średnicy w.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `color` | kolor | `white` | kolor |

### `<Sprite>`

Obrazek z <Resources> rozciągnięty do w × h.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `texture` | tekst | `""` | id tekstury z <Resources> |
| `tint` | kolor | `white` | zabarwienie (white = bez zmian) |

### `<Text>`

Napis. {Aktor.pole} wstawia wartość na żywo. W scenie 3D napisy zwykle mają space="screen".

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `value` | tekst | `""` | treść napisu |
| `size` | liczba całkowita | `20` | wielkość liter |
| `color` | kolor | `white` | kolor |

## Wbudowane: 3D

### `<Cube>`

Prostopadłościan w × h × d.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `color` | kolor | `white` | kolor |
| `wire` | true/false | `false` | true = tylko krawędzie (siatka) |

### `<Sphere>`

Kula o średnicy w.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `color` | kolor | `white` | kolor |
| `wire` | true/false | `false` | true = tylko krawędzie (siatka) |

### `<Cylinder>`

Walec: średnica w, wysokość h.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `color` | kolor | `white` | kolor |
| `wire` | true/false | `false` | true = tylko krawędzie (siatka) |

### `<Plane>`

Płaska podłoga w × d.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `color` | kolor | `white` | kolor |

### `<Model>`

Model 3D (.glb, .gltf, .obj) z <Resources>.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `model` | tekst | `""` | id modelu z <Resources> |
| `tint` | kolor | `white` | zabarwienie |

## Wbudowane: kamera i światło

### `<Camera>`

Kamera. Pozycja z Transform aktora. Pierwsza aktywna kamera w scenie jest używana.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `mode` | fixed \| follow \| orbit \| fps | `fixed` | fixed | follow (2D i 3D), orbit | fps (3D) |
| `target` | tekst | `""` | aktor, za którym podąża / na który patrzy |
| `zoom` | liczba | `1` | przybliżenie (2D) |
| `fov` | liczba | `60` | kąt widzenia w stopniach (3D) |
| `distance` | liczba | `8` | odległość od celu (3D follow/orbit) |
| `height` | liczba | `4` | wysokość nad celem (3D follow/orbit) |
| `active` | true/false | `true` | czy ta kamera jest używana |

### `<Light>`

Światło w scenie 3D (maks. 4). Pozycja z Transform.

| Pole | Typ | Domyślnie | Opis |
|------|-----|-----------|------|
| `kind` | point \| directional \| ambient | `point` | point = żarówka, directional = słońce (świeci od swojej pozycji do (0, 0, 0)), ambient = światło otoczenia (bez kierunku) |
| `color` | kolor | `white` | kolor światła |
| `intensity` | liczba | `1` | jasność |
