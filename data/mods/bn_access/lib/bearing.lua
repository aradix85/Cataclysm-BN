-- Where something is, relative to you.
--
-- Pure arithmetic in, words out. No game state, so every bearing is assertable.
--
-- Eight compass points and a step count, never coordinates (P3). Coordinates
-- have to be turned into a direction in the player's head; a direction does not.

local bearing = {}

-- Screen and map coordinates run x east and y south, so north is negative y.
-- The order below starts at east and turns anticlockwise, which is the order
-- the angle produces.
local POINTS = { "east", "northeast", "north", "northwest", "west", "southwest", "south", "southeast" }

--- The compass point from you to an offset.
---
--- Snapped by angle rather than by the sign of each axis: ten squares east and
--- one south is east, not southeast, which is what a player would say and what
--- keeps a long approach from wobbling between two words.
--- @param dx integer squares east, negative for west
--- @param dy integer squares south, negative for north
--- @return string|nil nil when the offset is zero, which has no direction
bearing.of = function(dx, dy)
  if dx == 0 and dy == 0 then return nil end
  local angle = math.atan(-dy, dx)
  local eighth = math.pi / 4
  local index = math.floor((angle + eighth / 2) / eighth) % 8
  return POINTS[index + 1]
end

--- How many steps away, counting a diagonal as one step, as the game does.
bearing.distance = function(dx, dy) return math.max(math.abs(dx), math.abs(dy)) end

--- "4 northeast", or nil when the offset is zero.
bearing.describe = function(dx, dy)
  local point = bearing.of(dx, dy)
  if not point then return nil end
  return string.format("%d %s", bearing.distance(dx, dy), point)
end

return bearing
