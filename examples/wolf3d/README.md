# Wolf 3D na PimpEx

Prosty klon Wolfensteina 3D: jeden poziom, 7 strażników, drzwi, apteczki,
amunicja, skarby i winda (zielone pole) na końcu.

```
pimpex dev examples/wolf3d
```

| Klawisz                  | Akcja                         |
|--------------------------|-------------------------------|
| strzałki góra/dół, W/S   | przód / tył                   |
| strzałki lewo/prawo, mysz| obrót                         |
| A / D                    | krok w bok                    |
| SHIFT                    | bieg                          |
| SPACJA                   | strzał                        |
| E                        | otwieranie drzwi              |
| 9 / 8                    | kody: nieśmiertelność / amunicja (z `library.swap`) |

## Jak to jest zrobione

- **Poziom** to mapa ASCII w `scenes/poziom1/scene.xml` (`<Tilemap>`). `#`
  i `B` to ściany, `D` drzwi, `G` strażnik, `+` apteczka, `a` amunicja,
  `$` skarb, `E` winda, `P` gracz. Zmień mapę i zapisz, a gra wczyta ją na nowo.
- **Gracz** (`scripts/behaviors/wolf_player.lua`) chodzi przez prędkość
  (`Body`), więc ściany z `Collider` go zatrzymują. Strzela promieniem:
  `self:look(40)` zwraca pierwszego trafionego aktora.
- **Strażnik** (`guard.lua`) patrzy przez `self:sees("Player")`, więc ściany
  zasłaniają widok. Słyszy strzały w promieniu 14 m, podchodzi, obraca się
  (`self:face`) i strzela. Po śmierci zostawia ciało i amunicję.
- **Drzwi** (`door.lua`) wjeżdżają w sufit po naciśnięciu E, a na czas
  otwarcia usuwają swój `Collider`.
- **HUD i koniec gry** są w `scenes/poziom1/scene.swap`, zwykłe procedury `.swap`.
