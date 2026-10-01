-- Player: arrows/W/S – walk and turn, A/D – strafe, SHIFT – run,
-- mouse – turn, SPACE – shoot, E – open doors.
-- Parameters: speed (0.12), turn – degrees per frame (3), mouse – mouse sensitivity (0.12).
return {
  on_start = function(self)
    self.cooldown = 0
  end,

  on_update = function(self)
    local turn = self.turn or 3
    if key("LEFT") then self.ry = self.ry + turn end
    if key("RIGHT") then self.ry = self.ry - turn end
    self.ry = self.ry - get("Mouse", "dx") * (self.mouse or 0.12)

    -- Move through velocity so walls (Colliders) stop the player.
    local f, s = 0, 0
    if key("UP") or key("W") then f = f + 1 end
    if key("DOWN") or key("S") then f = f - 1 end
    if key("D") then s = s + 1 end
    if key("A") then s = s - 1 end
    local speed = (self.speed or 0.12) * (key("SHIFT") and 1.6 or 1)
    local yaw = math.rad(self.ry)
    self.vx = (-math.sin(yaw) * f + math.cos(yaw) * s) * speed
    self.vz = (-math.cos(yaw) * f - math.sin(yaw) * s) * speed

    -- Shooting: a ray straight ahead, the first actor hit.
    if self.cooldown > 0 then self.cooldown = self.cooldown - 1 end
    if pressed("SPACE") and self.cooldown == 0 then
      self.cooldown = 12
      local ammo = get("Game", "ammo")
      if ammo <= 0 then
        message("Out of ammo!")
      else
        set("Game", "ammo", ammo - 1)
        set("Game", "muzzle", 6)
        wolf_noise = { x = self.x, z = self.z, frame = frame() }
        local hit, dist = self:look(40)
        if hit and has(hit, "Health") then
          local damage = math.random(8, 16)
          if dist < 4 then damage = damage + 8 end -- close range hurts more
          set(hit, "Health.hp", get(hit, "Health.hp") - damage)
        end
      end
    end

    -- Doors: E when one is right in front of the player.
    if pressed("E") then
      local hit = self:look(3)
      if hit and has_tag(hit, "door") then
        set(hit, "open", 1)
      end
    end

    wolf_result.score = get("Game", "score")
    wolf_result.kills = get("Game", "kills")
    wolf_result.guards = get("Game", "guards")
  end,
}
