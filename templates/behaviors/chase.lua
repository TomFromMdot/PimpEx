-- Chases an actor.
-- Parameters: target (default Player), speed (default 1),
--             axes – which axes to chase on: "xy" (2D, default) or "xz" (3D, along the ground),
--             range – only chase when the target is closer than range (optional).
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
