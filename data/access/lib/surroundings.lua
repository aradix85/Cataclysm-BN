-- Turning what was collected around the player into what is said about it.
--
-- Pure: a table in, a list of strings out. No game state, no gapi.
--
-- What is left here is the frame of the room and nothing else: whether she can see,
-- and how far each way lets her walk. What is standing and lying about used to be
-- grouped and spoken here too, and that was a second reading of a world the game
-- already lists on its own key -- which is what made one question feel like three
-- systems. Creatures and items are the game's own list; the ways out are the rows
-- this frame sits on top of.

local surroundings = {}

--- How far the room goes, as a compass sweep.
---
--- The one answer here whose subject is the direction rather than the thing, and
--- deliberately: these four are a frame rather than four separate findings, and a
--- frame is only usable if it always arrives in the same order. So it reads north,
--- east, south, west, always, and the number is how many squares can be walked that
--- way before something stops her.
---
--- A sighted player has this for free -- the whole room is on the screen, and the
--- shape of it needs no remembering. Without that, a corridor and an open field
--- feel identical until something is walked into, which is what makes this the
--- expensive thing to work out and the cheap thing to say.
--- @param reach table|nil { north = { steps, blocked }, east = ..., south = ..., west = ... }
--- @return string|nil
local function reach_line(reach)
  if not reach then return nil end

  local parts = {}
  for _, side in ipairs({ "north", "east", "south", "west" }) do
    local arm = reach[side]
    if arm then
      if arm.unknown then
        -- She has not been far enough that way to know, and saying "open" would
        -- be telling her the map rather than what she has seen.
        parts[#parts + 1] = string.format("%s %d or more", side, arm.steps)
      elseif not arm.blocked then
        -- Nothing stopped her within the distance looked at, which is a different
        -- fact from a long corridor and is said as one.
        parts[#parts + 1] = side .. " open"
      elseif arm.steps == 0 then
        parts[#parts + 1] = side .. " blocked"
      else
        parts[#parts + 1] = string.format("%s %d", side, arm.steps)
      end
    end
  end

  if #parts == 0 then return nil end
  return "Space: " .. table.concat(parts, ", ") .. "."
end

--- That she cannot see, said before anything she cannot see is reported.
---
--- The one sentence here whose subject is her own eyes rather than the world, and
--- it earns that: unlit is why every other answer stops one square out. Without it
--- the room reads as four unexplored directions, which is a different thing
--- entirely -- one is a place she has not been, the other is a place she is standing
--- in and cannot see. A sighted player is told this by a black screen and never has
--- to ask.
---
--- Said about the world rather than about her sight range, so it stays true for a
--- character who can see in the dark: the place is unlit either way.
--- @param around table { dark = boolean|nil }
--- @return string|nil
local function dark_line(around)
  if not around.dark then return nil end
  return "Dark."
end

--- How far the room goes, said before the list of what is in it.
---
--- Everything else about the surroundings is a list now -- enemies, creatures, the
--- ways out -- because a list can be walked and a sentence goes past once. This
--- stays a sentence: it is not four findings but one frame, and a frame has to
--- arrive whole and in the same order every time.
--- @param around table { reach = { north = { steps, blocked, unknown }, ... } }
--- @return string[]
surroundings.space = function(around)
  local out = {}

  local dark = dark_line(around)
  if dark then out[#out + 1] = dark end

  local line = reach_line(around.reach)
  if line then out[#out + 1] = line end

  return out
end

return surroundings
