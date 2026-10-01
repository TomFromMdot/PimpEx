# GUI – menu, pauza, przyciski

GUI to ekrany z przyciskami i napisami rysowane **nad grą**, w pikselach
okna. Każdy plik `gui/<nazwa>.xml` w katalogu projektu to jedna **scena GUI**,
np. `gui/menu.xml`, `gui/pauza.xml`, `gui/hud.xml`. Włącza się ją po nazwie:

| Skąd                     | Jak                                                   |
|--------------------------|-------------------------------------------------------|
| `scene.swap`             | `GUI_SHOW pauza`, `GUI_HIDE pauza`, `GUI_TOGGLE pauza` |
| przycisk GUI             | `click="GUI_HIDE pauza; SCENE menu"`                  |
| Lua                      | `gui.show("pauza")`, `gui.hide`, `gui.toggle`, `gui.shown` |
| `scene.xml`              | `<Scene gui="hud">` – GUI widoczne od startu sceny    |

Sceny GUI należą do całego projektu, a nie do jednej sceny gry. Może być
widocznych kilka naraz (np. `hud` i `pauza`). Ostatnio pokazana jest na
wierzchu. Przy każdym wczytaniu sceny (`SCENE`) widać dokładnie te GUI, które
podaje jej `<Scene gui="...">`, a pozostałe się chowają.

```xml
<!-- gui/pauza.xml -->
<Gui pause="true" dim="#00000099">
  <Panel  anchor="center" w="340" h="280" color="#1c1c2c" border="#ffffff40"/>
  <Label  anchor="center" y="-90" text="Pauza" size="44" color="gold"/>
  <Button anchor="center" y="-10" text="Wróć" key="ESCAPE" click="GUI_HIDE pauza"/>
  <Button anchor="center" y="60"  text="Od nowa" click="GUI_HIDE pauza; SCENE main"/>
</Gui>
```

```
# scene.swap
PROC menu_pauzy
  IF PRESSED ESCAPE : GUI_SHOW pauza
END
```

## `<Gui>`

| Atrybut | Znaczenie                                                                 |
|---------|---------------------------------------------------------------------------|
| `pause` | `true` – gdy to GUI jest widoczne, gra stoi (procedury, Lua i fizyka). Kliknięcia działają dalej. |
| `dim`   | kolor przyciemnienia całego okna pod GUI, np. `#00000099`                 |

## Elementy

Elementy rysują się w kolejności z pliku (późniejszy na wierzchu).

| Element    | Co to jest                       | Własne atrybuty                                      |
|------------|----------------------------------|------------------------------------------------------|
| `<Panel>`  | prostokąt (tło okienka)          | `color` (domyślnie półprzezroczysty czarny), `border` |
| `<Label>`  | napis                            | `text`, `size`, `color`, `align` (`left`, `center`, `right`) |
| `<Button>` | przycisk                         | `text`, `size`, `color`, `hover`, `text_color`, `border`, `click`, `key` |
| `<Image>`  | obrazek                          | `src` (plik względem katalogu projektu), `color` (odcień) |

Wspólne atrybuty:

| Atrybut  | Znaczenie                                                                |
|----------|--------------------------------------------------------------------------|
| `anchor` | punkt okna, od którego liczy się pozycja: `top-left` (domyślnie), `top`, `top-right`, `left`, `center`, `right`, `bottom-left`, `bottom`, `bottom-right` |
| `x`, `y` | pozycja **środka** elementu względem kotwicy (piksele, `y` rośnie w dół) |
| `w`, `h` | szerokość i wysokość (przycisk: domyślnie 220 × 56)                      |
| `name`   | nazwa elementu (tylko do opisu)                                          |

Kotwica trzyma układ po zmianie rozmiaru okna: `anchor="center"` zostaje na
środku, a `anchor="bottom-right" x="-80" y="-30"` w prawym dolnym rogu.

`text` może zawierać wartości pól, tak jak `TEXT` w `.swap`:
`text="Punkty: {Game.score}"`.

## Przyciski

- `click` – instrukcje `.swap` oddzielone średnikiem, wykonywane po
  kliknięciu: `click="SET Game.level 2; GUI_HIDE menu; SCENE poziom"`.
  Działa każda instrukcja z docs/opcodes.md oprócz `PROC_ADD`, także `CALL`
  do Lua. Bez `SELF` i `OTHER`.
- `key` – klawisz, który działa jak kliknięcie, np. `key="ESCAPE"` na
  przycisku „Wróć” albo `key="ENTER"` na „Graj”.
- Kliknięcie to wciśnięcie i puszczenie lewego przycisku myszy na tym samym
  przycisku. Pod kursorem przycisk dostaje kolor `hover`.
- Panel i przycisk zasłaniają to, co pod nimi. GUI z `pause="true"`
  przykrywa wszystkie niższe GUI.

Gdy widać GUI z przyciskami, kamera fps oddaje kursor, żeby dało się
w nie klikać.

## Testy bez okna

`--input` przyjmuje kliknięcia: `CLICK@x/y:klatka` (albo `CLICK@x/y:od-do`)
stawia kursor w `(x, y)` i trzyma lewy przycisk myszy:

```
pimpex play . --headless --frames 30 --input "CLICK@400/300:5"
```

Linia `gui:` w `.pimpex/state.txt` pokazuje widoczne GUI (z dopiskiem
`(pauza)`, gdy gra stoi).

## Sprawdzanie

`pimpex check` sprawdza pliki `gui/*.xml`, instrukcje w `click`, nazwy GUI
w `GUI_SHOW`/`GUI_HIDE`/`GUI_TOGGLE` i w `<Scene gui>`. Po zapisaniu pliku
GUI w trakcie gry (`pimpex dev`) silnik podmienia je w locie.
