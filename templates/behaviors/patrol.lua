-- Walks back and forth.
-- Parameters: axis – x, y or z (default x),
--             distance – how far from the start (default 100), speed (default 2).
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
