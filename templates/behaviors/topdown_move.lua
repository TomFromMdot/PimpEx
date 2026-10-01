-- Top-down movement (like in RPGs): arrows or WASD.
-- Parameters: speed (default 4).
-- With <Collider/> the actor cannot walk through walls (other solid Colliders).
--   <Behavior name="topdown_move" speed="4"/>
return {
  on_update = function(self)
    local speed = self.speed or 4
    local dx, dy = 0, 0
    if key("LEFT") or key("A") then dx = dx - 1 end
    if key("RIGHT") or key("D") then dx = dx + 1 end
    if key("UP") or key("W") then dy = dy - 1 end
    if key("DOWN") or key("S") then dy = dy + 1 end
    if dx ~= 0 and dy ~= 0 then -- diagonals are not faster
      dx, dy = dx * 0.7071, dy * 0.7071
    end
    self.vx = dx * speed
    self.vy = dy * speed
  end,
}
