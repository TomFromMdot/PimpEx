-- Platformówka: strzałki/A-D w bok, SPACJA/W/strzałka w górę – skok.
-- Parametry: speed (domyślnie 4), jump – siła skoku (domyślnie 10).
-- Aktor potrzebuje grawitacji i Collidera:
--   <Actor name="Player" gravity="0.6"> <Collider/> <Behavior name="platformer_move"/> </Actor>
return {
  on_update = function(self)
    local dir = 0
    if key("LEFT") or key("A") then dir = dir - 1 end
    if key("RIGHT") or key("D") then dir = dir + 1 end
    self.vx = dir * (self.speed or 4)
    -- Skakać można tylko stojąc na czymś (Body.grounded ustawia fizyka).
    if self.grounded == 1 and (pressed("SPACE") or pressed("UP") or pressed("W")) then
      self.vy = -(self.jump or 10) -- w 2D „w górę” to ujemne y
    end
  end,
}
