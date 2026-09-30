-- Widok z oczu (FPS): mysz – rozglądanie się, WASD – chodzenie, SPACJA – skok.
-- Parametry: speed (0.12), sensitivity – czułość myszy (0.15), jump (0.3),
--            camera – nazwa aktora z <Camera mode="fps" target="ten aktor"/>.
--   <Behavior name="fps_controller" camera="Cam"/>
return {
  on_update = function(self)
    local sens = self.sensitivity or 0.15
    self.ry = self.ry - get("Mouse", "dx") * sens
    if self.camera and exists(self.camera) then
      local pitch = get(self.camera, "rx") - get("Mouse", "dy") * sens
      set(self.camera, "rx", math.max(-80, math.min(80, pitch)))
    end
    local f, s = 0, 0
    if key("W") or key("UP") then f = f + 1 end
    if key("S") or key("DOWN") then f = f - 1 end
    if key("D") or key("RIGHT") then s = s + 1 end
    if key("A") or key("LEFT") then s = s - 1 end
    local yaw, speed = math.rad(self.ry), self.speed or 0.12
    self.vx = (-math.sin(yaw) * f + math.cos(yaw) * s) * speed
    self.vz = (-math.cos(yaw) * f - math.sin(yaw) * s) * speed
    if self.grounded == 1 and pressed("SPACE") then self.vy = self.jump or 0.3 end
  end,
}
