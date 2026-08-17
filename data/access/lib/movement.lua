-- What a step tells you.
--
-- Pure: a description of the square in, utterances out.
--
-- The game says nothing when a move is refused: it assumes you saw the wall.
-- Without sight that leaves a keypress with no answer at all, which cannot be
-- told apart from a key that never arrived -- and pressing harder is what a
-- player does next. So a blocked step always speaks, and names what stopped it.
--
-- A step that works speaks only when the ground changes. Announcing every step
-- across one floor is the flood F5 warns about, and it would bury the one case
-- that matters.

local movement = {}

--- @param step table { blocked = boolean|nil, name = string, changed = boolean|nil, area = string|nil }
--- @return string[]
movement.utterances = function(step)
  if step.blocked then
    -- Name first (P2): what is in the way is the answer, "blocked" is the
    -- qualifier, and the player can stop listening after the first word.
    return { string.format("%s, blocked.", step.name) }
  end

  local out = {}
  -- Crossing into another region of the overmap, which is the one thing about a
  -- step that is bigger than the square it lands on: leaving the woods and
  -- entering a cabin is where she is, not what is underfoot. A region is two
  -- dozen squares across, so this is rare by construction, and it is said before
  -- the ground because it is the larger of the two.
  local area = step.area or ""
  if area ~= "" then out[#out + 1] = area:sub(1, 1):upper() .. area:sub(2) .. "." end

  if step.changed and step.name ~= "" then out[#out + 1] = string.format("%s.", step.name) end
  return out
end

return movement
