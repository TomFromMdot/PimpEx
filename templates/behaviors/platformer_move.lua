-- Platformer: arrows/A-D to move, SPACE/W/up arrow – jump.
-- Parameters: speed (default 4), jump – jump strength (default 10).
-- The actor needs gravity and a Collider:
--   <Actor name="Player" gravity="0.6"> <Collider/> <Behavior name="platformer_move"/> </Actor>
return {
  on_update = function(self)
    local dir = 0
    if key("LEFT") or key("A") then dir = dir - 1 end
    if key("RIGHT") or key("D") then dir = dir + 1 end
    self.vx = dir * (self.speed or 4)
    -- Jump only while standing on something (physics sets Body.grounded).
    if self.grounded == 1 and (pressed("SPACE") or pressed("UP") or pressed("W")) then
      self.vy = -(self.jump or 10) -- in 2D "up" is negative y
    end
  end,
}
