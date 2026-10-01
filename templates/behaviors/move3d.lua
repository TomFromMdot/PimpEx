-- 3D movement (third person): W/S or up/down arrows – forward/back,
-- A/D or left/right arrows – turn, SPACE – jump.
-- Parameters: speed (0.15), turn – degrees per frame (3), jump (0.3).
-- Goes with: <Camera mode="follow" target="Player"/>
return {
  on_update = function(self)
    if key("A") or key("LEFT") then self.ry = self.ry + (self.turn or 3) end
    if key("D") or key("RIGHT") then self.ry = self.ry - (self.turn or 3) end
    local forward = 0
    if key("W") or key("UP") then forward = forward + 1 end
    if key("S") or key("DOWN") then forward = forward - 1 end
    local yaw = math.rad(self.ry)
    local speed = (self.speed or 0.15) * forward
    self.vx = -math.sin(yaw) * speed
    self.vz = -math.cos(yaw) * speed
    if self.grounded == 1 and pressed("SPACE") then
      self.vy = self.jump or 0.3 -- in 3D "up" is positive y
    end
  end,
}
