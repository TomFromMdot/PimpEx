-- Spins around.
-- Parameters: axis – rz in 2D, ry in 3D (default ry), speed – degrees per frame (default 2).
--   <Behavior name="spin" axis="ry" speed="3"/>
return {
  on_update = function(self)
    local a = self.axis or "ry"
    self[a] = (self[a] + (self.speed or 2)) % 360
  end,
}
