-- Guard: stands still until it sees the player or hears a shot.
-- Then it walks to where the player was last seen and shoots on sight.
-- Parameters: speed (0.05), range – sight distance (16), damage – maximum damage (10).

local function die(self)
  set("Game", "score", get("Game", "score") + 100)
  set("Game", "kills", get("Game", "kills") + 1)
  spawn("DeadGuard", nil, self.x, 0.05, self.z)
  spawn("AmmoBox", nil, self.x + 0.6, 0.15, self.z)
  message("Guard down! +100")
  self:destroy()
end

return {
  on_start = function(self)
    self.state = "idle"
    self.cooldown = 60
  end,

  on_update = function(self)
    if self:get("Health.hp") <= 0 then return die(self) end

    local sees = self:sees("Player", self.range or 16)
    if sees then
      self.state = "attack"
      self.target_x, self.target_z = get("Player", "x"), get("Player", "z")
    elseif self.state == "idle" and wolf_noise and frame() - wolf_noise.frame < 5 then
      local dx, dz = wolf_noise.x - self.x, wolf_noise.z - self.z
      if dx * dx + dz * dz < 14 * 14 then -- heard the shot
        self.state = "hunt"
        self.target_x, self.target_z = wolf_noise.x, wolf_noise.z
      end
    end

    if self.state == "idle" then
      self.vx, self.vz = 0, 0
      return
    end

    -- Walk to the target; keep 3 m away while the player is in sight.
    local dx, dz = self.target_x - self.x, self.target_z - self.z
    local dist = math.sqrt(dx * dx + dz * dz)
    local speed = self.speed or 0.05
    if dist > (sees and 3 or 0.3) then
      self.vx, self.vz = dx / dist * speed, dz / dist * speed
    else
      self.vx, self.vz = 0, 0
      if not sees then self.state = "idle" end -- arrived, nobody here
    end

    if sees then
      self:face("Player")
      self.cooldown = self.cooldown - 1
      if self.cooldown <= 0 then
        self.cooldown = math.random(50, 90)
        local d = self:distance("Player")
        if math.random() < math.max(0.25, 0.9 - d / 20) then -- harder to hit from far away
          local hp = get("Player", "Health.hp") - math.random(4, self.damage or 10)
          set("Player", "Health.hp", math.max(0, hp))
          set("Game", "hurt", 10)
        end
      end
    else
      self.cooldown = math.max(self.cooldown, 20) -- reaction time after spotting the player
    end
  end,
}
