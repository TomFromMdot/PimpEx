-- Shared game functions (globals – every behavior can use them).

-- A message in the top-left corner for 2 seconds.
function message(msg)
  text("Msg", msg)
  set("Game", "msgt", 120)
end

-- The result, kept between scenes (the end screen shows it).
wolf_result = wolf_result or { score = 0, kills = 0, guards = 0 }

-- The player's last shot – guards nearby "hear" it.
wolf_noise = nil
