-- Gracz: strzałki/W/S – chodzenie i obrót, A/D – krok w bok, SHIFT – bieg,
-- mysz – obrót, SPACJA – strzał, E – otwieranie drzwi.
-- Parametry: speed (0.12), turn – stopnie na klatkę (3), mouse – czułość myszy (0.12).
return {
  on_start = function(self)
    self.cooldown = 0
  end,

  on_update = function(self)
    -- Obrót.
    local turn = self.turn or 3
    if key("LEFT") then self.ry = self.ry + turn end
    if key("RIGHT") then self.ry = self.ry - turn end
    self.ry = self.ry - get("Mouse", "dx") * (self.mouse or 0.12)

    -- Ruch przez prędkość, żeby ściany (Collider) zatrzymywały gracza.
    local f, s = 0, 0
    if key("UP") or key("W") then f = f + 1 end
    if key("DOWN") or key("S") then f = f - 1 end
    if key("D") then s = s + 1 end
    if key("A") then s = s - 1 end
    local speed = (self.speed or 0.12) * (key("SHIFT") and 1.6 or 1)
    local yaw = math.rad(self.ry)
    self.vx = (-math.sin(yaw) * f + math.cos(yaw) * s) * speed
    self.vz = (-math.cos(yaw) * f - math.sin(yaw) * s) * speed

    -- Strzał: promień prosto przed siebie, pierwszy trafiony aktor.
    if self.cooldown > 0 then self.cooldown = self.cooldown - 1 end
    if pressed("SPACE") and self.cooldown == 0 then
      self.cooldown = 12
      local ammo = get("Game", "ammo")
      if ammo <= 0 then
        message("Brak amunicji!")
      else
        set("Game", "ammo", ammo - 1)
        set("Game", "muzzle", 6)
        wolf_noise = { x = self.x, z = self.z, frame = frame() }
        local hit, dist = self:look(40)
        if hit and has(hit, "Health") then
          local damage = math.random(8, 16)
          if dist < 4 then damage = damage + 8 end -- z bliska boli bardziej
          set(hit, "Health.hp", get(hit, "Health.hp") - damage)
        end
      end
    end

    -- Drzwi: E, gdy stoją tuż przed graczem.
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
