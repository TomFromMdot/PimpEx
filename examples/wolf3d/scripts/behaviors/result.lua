-- End screen: shows the result kept in wolf_result.
return {
  on_start = function(self)
    local r = wolf_result or { score = 0, kills = 0, guards = 0 }
    self:text(string.format("Score: %d   Guards: %d/%d", r.score, r.kills, r.guards))
  end,
}
