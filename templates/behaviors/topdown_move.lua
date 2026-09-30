-- Ruch z góry (jak w grach RPG): strzałki albo WASD.
-- Parametry: speed – prędkość (domyślnie 4).
-- Z <Collider/> aktor nie przejdzie przez ściany (inne solidne Collidery).
--   <Behavior name="topdown_move" speed="4"/>
return {
  on_update = function(self)
    local speed = self.speed or 4
    local dx, dy = 0, 0
    if key("LEFT") or key("A") then dx = dx - 1 end
    if key("RIGHT") or key("D") then dx = dx + 1 end
    if key("UP") or key("W") then dy = dy - 1 end
    if key("DOWN") or key("S") then dy = dy + 1 end
    if dx ~= 0 and dy ~= 0 then -- po skosie nie szybciej niż prosto
      dx, dy = dx * 0.7071, dy * 0.7071
    end
    self.vx = dx * speed
    self.vy = dy * speed
  end,
}
