-- Znajdźka: gracz podnosi ją, wchodząc na nią.
-- Parametry: kind – health | ammo | score, amount – ile.
return {
  on_update = function(self)
    if not self:touching("Player") then return end
    local kind, amount = self.kind or "score", self.amount or 10
    if kind == "health" then
      local hp, max = get("Player", "Health.hp"), get("Player", "Health.max")
      if hp >= max then return end -- pełne życie – zostaje na później
      set("Player", "Health.hp", math.min(max, hp + amount))
      message("+" .. amount .. " życia")
    elseif kind == "ammo" then
      set("Game", "ammo", get("Game", "ammo") + amount)
      message("+" .. amount .. " naboi")
    else
      set("Game", "score", get("Game", "score") + amount)
      message("+" .. amount .. " punktów")
    end
    self:destroy()
  end,
}
