-- Wspólne funkcje gry (globalne – widzą je wszystkie zachowania).

-- Napis w lewym górnym rogu na 2 sekundy.
function message(msg)
  text("Msg", msg)
  set("Game", "msgt", 120)
end

-- Wynik zapamiętany między scenami (ekran końca go pokazuje).
wolf_result = wolf_result or { score = 0, kills = 0, guards = 0 }

-- Ostatni strzał gracza – strażnicy w pobliżu go „słyszą”.
wolf_noise = nil
