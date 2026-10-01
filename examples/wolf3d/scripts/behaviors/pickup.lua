-- Pickup: the player collects it by walking over it.
-- Parameters: kind – health | ammo | score, amount – how much.
return {
  on_update = function(self)
    if not self:touching("Player") then return end
    local kind, amount = self.kind or "score", self.amount or 10
    if kind == "health" then
      local hp, max = get("Player", "Health.hp"), get("Player", "Health.max")
      if hp >= max then return end -- full health – leave it for later
      set("Player", "Health.hp", math.min(max, hp + amount))
      message("+" .. amount .. " health")
    elseif kind == "ammo" then
      set("Game", "ammo", get("Game", "ammo") + amount)
      message("+" .. amount .. " ammo")
    else
      set("Game", "score", get("Game", "score") + amount)
      message("+" .. amount .. " points")
    end
    self:destroy()
  end,
}
