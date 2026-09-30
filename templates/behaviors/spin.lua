-- Kręci się w kółko.
-- Parametry: axis – rz w 2D, ry w 3D (domyślnie ry), speed – stopnie na klatkę (domyślnie 2).
--   <Behavior name="spin" axis="ry" speed="3"/>
return {
  on_update = function(self)
    local a = self.axis or "ry"
    self[a] = (self[a] + (self.speed or 2)) % 360
  end,
}
