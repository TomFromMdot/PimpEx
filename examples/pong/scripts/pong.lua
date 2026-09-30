-- Serwis: piłka wraca na środek i leci w stronę gracza, który stracił punkt.
-- kierunek: 1 = w prawo, -1 = w lewo
function serwis(kierunek)
  set("Pilka", "x", 400)
  set("Pilka", "y", 300)
  set("Pilka", "vx", 5 * kierunek)
  set("Pilka", "vy", math.random(-3, 3))
  log("Wynik: " .. get("Game", "lewy") .. " : " .. get("Game", "prawy"))
end
