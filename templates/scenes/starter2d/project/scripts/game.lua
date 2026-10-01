-- Lua scripts: more complex logic, called from scene.swap with CALL.
-- At the top level of the file we only define functions and variables.

-- `sparks or {}` – the spark list survives a script hot-reload.
sparks = sparks or {}

-- CALL coin_collected – the player touched a coin.
function coin_collected()
  local x, y = get("Coin", "x"), get("Coin", "y")
  for i = 1, 8 do
    local name = spawn("Spark", nil, x, y)
    set(name, "vx", math.random(-40, 40) / 10)
    set(name, "vy", math.random(-40, 40) / 10)
    table.insert(sparks, name)
  end
  -- the coin jumps to a random place
  set("Coin", "x", math.random(40, 760))
  set("Coin", "y", math.random(60, 560))
  log("Coin! Total: " .. get("Game", "coins"))
end

-- CALL update_sparks – every frame the sparks fade and disappear.
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
