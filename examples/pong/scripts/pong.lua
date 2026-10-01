-- Serve: the ball returns to the centre and flies towards the player who lost the point.
-- direction: 1 = right, -1 = left
function serve(direction)
  set("Ball", "x", 400)
  set("Ball", "y", 300)
  set("Ball", "vx", 5 * direction)
  set("Ball", "vy", math.random(-3, 3))
  log("Score: " .. get("Game", "left") .. " : " .. get("Game", "right"))
end
