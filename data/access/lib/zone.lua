-- Turning the place she is standing in into speech.
--
-- Pure: a table in, a list of strings out. No game state, no gapi.
--
-- The question this answers is not "what is around me" but "where have I ended up,
-- and how much of it is there". A region tile is always 24 squares across, but a
-- place -- a station, a mall, a farm -- runs across several of them under one name,
-- and nothing in the game says so in words: a sighted player reads it off the
-- screen and off the map, in one glance, without noticing they did it.
--
-- The part that matters most is the last line. The map memory holds what she has
-- laid eyes on, so the difference between how far the place runs and how far she
-- has been is the part she has never met -- and saying which way that lies turns
-- "there is probably more" into somewhere to walk.
--
-- Steps and not tiles, because a step is the unit she moves in and a tile is a
-- number she would have to multiply by 24 in her head. The tile count is said too,
-- once, since it is how the same place reads on the overmap.

local text = require("./text")

local zone = {}

-- How much further the place has to run past what she has seen before that counts
-- as somewhere to go. Below this it is the edge of the room she is standing in,
-- and saying it would send her to walk into a wall she is already beside.
local UNSEEN_MARGIN = 6

--- Normalise one measurement.
--- @param report table as perception.zone_around_player returns it
--- @return table
zone.state = function(report)
  local function n(key) return report[key] or 0 end
  return {
    name = text.clean(report.name),
    tiles_wide = n("tiles_wide"),
    tiles_high = n("tiles_high"),
    reach = { north = n("reach_north"), east = n("reach_east"), south = n("reach_south"), west = n("reach_west") },
    seen = { north = n("seen_north"), east = n("seen_east"), south = n("seen_south"), west = n("seen_west") },
  }
end

--- The place and its size, as the heading of the list the ways out are read in.
---
--- No full stop: the reading model puts the number of rows after it, and a place
--- that names itself twice in one utterance is the noise this list exists to end.
--- Tiles because that is what the overmap calls the same place.
zone.title = function(state)
  local name = text.is_speakable(state.name) and state.name or "Here"
  name = name:sub(1, 1):upper() .. name:sub(2)

  local tiles = state.tiles_wide * state.tiles_high
  if tiles <= 1 then return name .. ", 1 tile" end
  return string.format("%s, %d by %d tiles", name, state.tiles_wide, state.tiles_high)
end

--- Which way there is still something to find.
---
--- Named directions rather than a figure: the number would have to be turned into a
--- direction in her head, and the direction is the whole of what the answer is for.
---
--- How far the place runs is not said beside it. It was, and it was a second compass
--- sweep in the same breath as the one that says how far the room lets her walk --
--- two answers that sound alike, mean different things and have to be told apart
--- while walking. The room's reach is the one she acts on; this says where there is
--- more of the place than she has met.
zone.unexplored = function(state)
  local out = {}
  for _, side in ipairs({ "north", "east", "south", "west" }) do
    if state.reach[side] - state.seen[side] >= UNSEEN_MARGIN then out[#out + 1] = side end
  end

  if #out == 0 then return "Seen all of it." end
  if #out == 1 then return string.format("Unexplored to the %s.", out[1]) end

  local last = table.remove(out)
  return string.format("Unexplored to the %s and %s.", table.concat(out, ", "), last)
end

return zone
