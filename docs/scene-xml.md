# Pliki projektu: `pimpex.xml` i `scene.xml`

## Układ projektu

```
moja_gra/
  pimpex.xml                 ustawienia gry: okno, scena startowa
  scenes/<scena>/scene.xml   aktorzy sceny (kto jest na planszy, jak wygląda)
  scenes/<scena>/scene.swap  procedury sceny (co się dzieje) – docs/opcodes.md
  scenes/<scena>/library.swap  procedury do wklejania na żądanie (opcjonalnie)
  scripts/*.lua              funkcje Lua wołane przez CALL – docs/lua-api.md
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

  <Actor name="Score" x="20" y="20" w="0" h="0" z="10">
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
| `name`     | wymagany  | unikalna nazwa (bez spacji, `.`, `/`, `:`)  |
| `x`, `y`   | 0         | pozycja **środka** aktora                   |
| `w`, `h`   | 32        | szerokość i wysokość (też prostokąt kolizji)|
| `vx`, `vy` | 0         | prędkość w pikselach na klatkę              |
| `rotation` | 0         | obrót w stopniach                           |
| `z`        | 0         | kolejność rysowania (większe = wyżej)       |
| `visible`  | true      | czy aktor jest rysowany                     |

### Wygląd i dane (elementy w środku aktora)

| Element                                    | Znaczenie                                       |
|--------------------------------------------|-------------------------------------------------|
| `<Rect color="..."/>`                      | prostokąt `w` × `h`                             |
| `<Circle color="..."/>`                    | koło o średnicy `w`                             |
| `<Sprite texture="id" tint="..."/>`        | obrazek z `<Resources>` rozciągnięty do `w`×`h` |
| `<Text value="..." size="20" color="..."/>`| napis; `{Aktor.pole}` wstawia wartość na żywo   |
| `<Var name="..." value="0"/>`              | zmienna aktora, np. życia, punkty               |

**Archetyp** to wzór aktora, który nie istnieje od początku. Tworzy go
`SPAWN` w `scene.swap` albo `spawn()` w Lua.

### Kolory

`#rgb`, `#rrggbb`, `#rrggbbaa` albo nazwy: `white`, `black`, `red`, `green`,
`blue`, `yellow`, `orange`, `purple`, `pink`, `gray`, `brown`, `cyan`, `gold`,
`transparent`. Po polsku: `biały`, `czarny`, `czerwony`, `zielony`,
`niebieski`, `żółty`, `pomarańczowy`, `fioletowy`, `różowy`, `szary`,
`brązowy`, `złoty`.
