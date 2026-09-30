-- Skrypty Lua: bardziej złożona logika, wywoływana z scene.swap przez CALL.
-- Na górnym poziomie pliku tylko definiujemy funkcje i zmienne.

-- `sparks or {}` – po hot-reloadzie skryptu lista iskier nie ginie.
sparks = sparks or {}

-- CALL coin_collected – gracz dotknął monety.
function coin_collected()
  local x, y = get("Coin", "x"), get("Coin", "y")
  for i = 1, 8 do
    local name = spawn("Spark", nil, x, y)
    set(name, "vx", math.random(-40, 40) / 10)
    set(name, "vy", math.random(-40, 40) / 10)
    table.insert(sparks, name)
  end
  -- moneta przeskakuje w losowe miejsce
  set("Coin", "x", math.random(40, 760))
  set("Coin", "y", math.random(60, 560))
  log("Moneta! Razem: " .. get("Game", "coins"))
end

-- CALL update_sparks – co klatkę iskry gasną i znikają.
function update_sparks()
  for i = #sparks, 1, -1 do
    local name = sparks[i]
    local life = get(name, "life") - 1
    set(name, "life", life)
    if life <= 0 then
      destroy(name)
      table.remove(sparks, i)
    end
  end
end
