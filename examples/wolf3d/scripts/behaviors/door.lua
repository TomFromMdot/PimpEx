-- Drzwi: gracz naciska E (ustawia zmienną open = 1), drzwi wjeżdżają
-- w sufit i znika ich Collider. Po `stay` klatkach zjeżdżają – chyba że
-- ktoś stoi w przejściu.
-- Parametry: stay – ile klatek zostają otwarte (180 = 3 s).

local function blocked(self)
  local reach = self.w / 2 + 0.4
  for _, name in ipairs(actors("@guard")) do
    if math.abs(get(name, "x") - self.x) < reach and math.abs(get(name, "z") - self.z) < reach then return true end
  end
  return math.abs(get("Player", "x") - self.x) < reach and math.abs(get("Player", "z") - self.z) < reach
end

return {
  on_start = function(self)
    self.base = self.y
    self.state = "closed"
  end,

  on_update = function(self)
    local speed = 0.08
    local wants = self:get("open") == 1
    if (self.state == "closed" or self.state == "closing") and wants then
      self.state = "opening"
    end

    if self.state == "opening" then
      self.y = self.y + speed
      if self.y >= self.base + self.h - 0.1 then
        self.state = "open"
        self.timer = self.stay or 180
        remove(self.name, "Collider") -- można przejść
      end
    elseif self.state == "open" then
      self.timer = self.timer - 1
      if self.timer <= 0 and not blocked(self) then
        self.state = "closing"
        self:set("open", 0)
        add(self.name, "Collider")
      end
    elseif self.state == "closing" then
      self.y = self.y - speed
      if self.y <= self.base then
        self.y = self.base
        self.state = "closed"
      end
    end
  end,
}
