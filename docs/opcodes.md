# Język `.swap` – procedury sceny

Plik `scene.swap` to **program pętli sceny**. Składa się z procedur (`PROC`).
Silnik wykonuje **każdą procedurę w każdej klatce** (60 razy na sekundę),
od góry do dołu, dopóki gra działa. To jest „nieskończony proces” sceny.

Plik możesz zmieniać **w trakcie gry**. Po zapisaniu silnik wczyta nową wersję
między klatkami, bez restartu. Jeśli w pliku jest błąd, silnik dalej wykonuje
poprzednią, poprawną wersję, a błąd zapisuje w `.pimpex/runtime.log`.

```
# To jest komentarz – silnik go pomija.
PROC ruch_gracza
  IF KEY LEFT  : MOVE Player -4 0
  IF KEY RIGHT : MOVE Player 4 0
END
```

## Procedury

| Zapis                 | Znaczenie                                                    |
|-----------------------|--------------------------------------------------------------|
| `PROC nazwa` … `END`  | wykonuje się co klatkę                                       |
| `PROC nazwa EVERY 30` | wykonuje się co 30 klatek (60 klatek = 1 sekunda)            |
| `PROC nazwa ONCE`     | wykonuje się raz, a potem silnik **sam usuwa ją z pliku**    |
| `PROC nazwa FOR @tag` | ciało wykonuje się dla **każdego** aktora z tagiem (`SELF`)  |
| `PROC nazwa FOR Health` | … dla każdego aktora z komponentem `Health`                |

Można łączyć tryby, np. `PROC regeneracja FOR Health EVERY 60`.

`ONCE` przydaje się do „wklejania” jednorazowych poleceń do działającej gry.
Dopisujesz na koniec pliku procedurę `ONCE`, zapisujesz, silnik ją wykonuje
i czyści.

Nazwy procedur muszą być unikalne w pliku.

## Wartości

Wszędzie, gdzie instrukcja oczekuje liczby, możesz wpisać:

- liczbę: `4`, `-2.5`
- pole aktora: `Player.x`, `Game.score`, `Player.Health.hp`
- pole z minusem: `-Ball.vx`
- losową liczbę: `RANDOM 1 6` (liczba z przedziału od 1 do 6, z ułamkami)

## Pola aktorów

| Pole        | Znaczenie                                              |
|-------------|--------------------------------------------------------|
| `x`, `y`    | pozycja środka aktora (piksele, `y` rośnie w dół)      |
| `vx`, `vy`  | prędkość – o tyle silnik przesuwa aktora co klatkę     |
| `w`, `h`    | szerokość i wysokość                                   |
| `rotation`  | obrót w stopniach                                      |
| `visible`   | `1` – widoczny, `0` – ukryty                           |
| `z`, `vz`, `d`, `rx`, `ry` | trzecia oś, prędkość, głębokość i obrót w 3D |
| `Komponent.pole` | pole komponentu, np. `Player.Health.hp`, `Cam.Camera.zoom` (docs/components.md) |
| inne nazwy  | Twoje zmienne, np. `Game.score`, `Player.lives`        |

Zmienna, której jeszcze nie ma, powstaje przy pierwszym `SET` albo `ADD`.
Odczyt nieistniejącej zmiennej daje `0`. Zmienne przechowują tylko liczby.
Pola tekstowe komponentów ustawia się tekstem w cudzysłowie:
`SET Hud.Text.value "Brawo!"`, `SET Player.Rect.color "red"`.

### SELF i OTHER

- `SELF` – w `PROC ... FOR` aktor, dla którego właśnie wykonuje się ciało:
  `MOVE SELF 1 0`, `IF SELF.Health.hp <= 0 : DESTROY SELF`.
- `OTHER` – aktor znaleziony przez ostatnie `TOUCH` z grupą:
  `IF TOUCH SELF @enemy : ADD OTHER.Health.hp -1`.

Pola specjalne (tylko do odczytu):

- `Screen.w`, `Screen.h` – rozmiar okna
- `Mouse.x`, `Mouse.y` – pozycja myszki, `Mouse.dx`, `Mouse.dy` – jej ruch w tej klatce
- `Scene.frame` – numer bieżącej klatki

## Instrukcje

| Instrukcja                              | Co robi                                                        |
|-----------------------------------------|----------------------------------------------------------------|
| `MOVE aktor dx dy [dz]`                 | przesuwa aktora o `dx`, `dy` (i `dz` w 3D)                     |
| `SET Aktor.pole wartość`                | ustawia pole (liczbą albo "tekstem" w cudzysłowie)             |
| `ADD Aktor.pole wartość`                | dodaje do pola (ujemna wartość odejmuje)                       |
| `MUL Aktor.pole wartość`                | mnoży pole (np. `MUL Ball.vx -1` odbija piłkę)                 |
| `SPAWN Archetyp [AS nazwa] [AT x y [z]]`| tworzy nowego aktora z archetypu opisanego w `scene.xml`       |
| `DESTROY aktor`                         | usuwa aktora                                                   |
| `FOLLOW aktor cel prędkość`             | przesuwa aktora w stronę celu (w 3D też po osi z)              |
| `TEXT aktor "tekst"`                    | zmienia napis aktora; `{Aktor.pole}` wstawia wartość pola      |
| `LOG "tekst"`                           | wypisuje tekst w konsoli i w `.pimpex/runtime.log`             |
| `CALL funkcja [argumenty…]`             | wywołuje funkcję Lua ze skryptów w katalogu `scripts/`         |
| `PROC_ADD nazwa`                        | wkleja procedurę `nazwa` z `library.swap` do `scene.swap`      |
| `PROC_REMOVE nazwa`                     | usuwa procedurę `nazwa` z `scene.swap`                         |
| `SCENE nazwa`                           | przełącza na inną scenę (`scenes/nazwa/`)                      |

`SPAWN` bez `AS` nadaje nazwę automatycznie (`Bullet_1`, `Bullet_2`…).
Bez `AT` nowy aktor pojawia się w pozycji z archetypu.

W `TEXT` zapis `{pole}` bez nazwy aktora oznacza pole tego samego aktora,
np. `TEXT Score "Punkty: {points}"`.

## Warunki: `IF`

```
IF warunek : instrukcja
IF warunek AND warunek : instrukcja
IF warunek : IF inny_warunek : instrukcja
```

| Warunek                   | Prawdziwy, gdy…                                      |
|---------------------------|------------------------------------------------------|
| `KEY W`                   | klawisz jest wciśnięty (trzymany)                    |
| `PRESSED SPACE`           | klawisz został wciśnięty w tej klatce (jednorazowo)  |
| `TOUCH a b`               | aktorzy `a` i `b` stykają się (prostokąty w, h, d)   |
| `TOUCH a @tag`            | `a` dotyka **któregokolwiek** aktora z tagiem – on staje się `OTHER` |
| `TOUCH a Komponent`       | jak wyżej, dla aktorów z komponentem                 |
| `EXISTS aktor`            | aktor istnieje                                       |
| `EXISTS @tag`             | istnieje choć jeden aktor z tagiem                   |
| `CHANCE 10`               | wylosowano 10% szans (w każdej klatce osobno)        |
| `wartość < wartość`       | porównanie: `<`, `<=`, `>`, `>=`, `==`, `!=`         |
| `NOT warunek`             | odwraca warunek                                      |

Klawisze: `A`–`Z`, `0`–`9`, `SPACE`, `ENTER`, `ESCAPE`, `SHIFT`,
`LEFT`, `RIGHT`, `UP`, `DOWN`.

## Biblioteka procedur: `library.swap`

Obok `scene.swap` może leżeć `library.swap`. Procedury z biblioteki **nie
wykonują się same**. To gotowe klocki, które silnik (boot) wkleja do
`scene.swap` na żądanie: przez `PROC_ADD nazwa` albo z Lua przez
`swap.add("nazwa")`. `PROC_REMOVE` i `swap.remove("nazwa")` wycinają procedurę
z `scene.swap`.

```
# library.swap
PROC deszcz EVERY 20
  SPAWN Kropla AT RANDOM 0 800 0
END
```

## Sprawdzanie

`pimpex check <projekt>` sprawdza wszystkie pliki bez uruchamiania gry
i wypisuje błędy w formacie `plik:linia: błąd: opis`.
