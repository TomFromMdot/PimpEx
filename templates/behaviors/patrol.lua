-- Chodzi tam i z powrotem.
-- Parametry: axis – oś ruchu: x, y albo z (domyślnie x),
--            distance – jak daleko od startu (domyślnie 100), speed (domyślnie 2).
--   <Behavior name="patrol" axis="x" distance="120" speed="2"/>
return {
  on_start = function(self)
    self.axis = self.axis or "x"
    self.origin = self[self.axis]
    self.dir = 1
  end,
  on_update = function(self)
    local a = self.axis
    self[a] = self[a] + (self.speed or 2) * self.dir
    if math.abs(self[a] - self.origin) >= (self.distance or 100) then
      self.dir = -self.dir
    end
  end,
}
