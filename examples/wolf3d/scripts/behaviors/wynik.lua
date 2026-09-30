-- Ekran końca: pokazuje wynik zapamiętany w wolf_result.
return {
  on_start = function(self)
    local r = wolf_result or { score = 0, kills = 0, guards = 0 }
    self:text(string.format("Punkty: %d   Strażnicy: %d/%d", r.score, r.kills, r.guards))
  end,
}
