-- Goni wskazanego aktora.
-- Parametry: target (domyślnie Player), speed (domyślnie 1),
--            axes – po których osiach goni: "xy" (2D, domyślnie) albo "xz" (3D, po ziemi),
--            range – goni tylko, gdy cel jest bliżej niż range (opcjonalnie).
--   <Behavior name="chase" target="Player" speed="1.5"/>
return {
  on_update = function(self)
    local target = self.target or "Player"
    if not exists(target) then return end
    local d, length = { x = 0, y = 0, z = 0 }, 0
    for axis in (self.axes or "xy"):gmatch(".") do
      d[axis] = get(target, axis) - self[axis]
      length = length + d[axis] * d[axis]
    end
    length = math.sqrt(length)
    if length < 0.001 or (self.range and length > self.range) then return end
    local step = math.min(self.speed or 1, length) / length
    self:move(d.x * step, d.y * step, d.z * step)
  end,
}
