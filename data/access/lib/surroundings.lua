-- Turning what was collected around the player into what is said about it.
--
-- Pure: a table in, a list of utterances out. No game state, no gapi.
--
-- Grouped by kind and never in map order (P4), enemies first because they are
-- the only thing that can kill you this turn. One utterance per group (F1), name
-- first within it (P2), and the whole thing bounded to a few lines (F5): this is
-- the overview, not the list.

local bearing = require("./bearing")

local surroundings = {}

--- @param entries table[] each { name, dx, dy }
--- @return table|nil the closest entry, nil when there are none
local function nearest_of(entries)
  local best, best_distance = nil, nil
  for _, entry in ipairs(entries) do
    local distance = bearing.distance(entry.dx, entry.dy)
    if not best_distance or distance < best_distance then
      best, best_distance = entry, distance
    end
  end
  return best
end

local function group_line(entries, singular, plural)
  local nearest = nearest_of(entries)
  -- Empty is silence (P5), and asking for the nearest is how that is known.
  if not nearest then return nil end
  local where = bearing.describe(nearest.dx, nearest.dy)
  local what = nearest.name

  if #entries == 1 then
    if where then return string.format("1 %s. %s, %s.", singular, what, where) end
    return string.format("1 %s. %s, here.", singular, what)
  end
  if where then return string.format("%d %s. Nearest: %s, %s.", #entries, plural, what, where) end
  return string.format("%d %s. Nearest: %s, here.", #entries, plural, what)
end

--- What to say about the surroundings.
---
--- Answering "nothing nearby" rather than staying silent is deliberate: P5 is
--- about not volunteering emptiness, and this was asked for. A silent answer to
--- a keypress cannot be told apart from a key that never arrived.
---
--- The area comes first because it is the largest thing true of where she is
--- standing, and because it is the answer to a question that otherwise costs a
--- trip to the overmap and back: the game names the region a square belongs to
--- and never says it out loud anywhere in play.
--- @param around table { area, enemies, others, landmarks }, each list of { name, dx, dy }
--- @return string[]
surroundings.overview = function(around)
  local out = {}

  local area = around.area or ""
  if area ~= "" then out[#out + 1] = area:sub(1, 1):upper() .. area:sub(2) .. "." end

  local enemies = group_line(around.enemies or {}, "enemy", "enemies")
  if enemies then out[#out + 1] = enemies end

  local others = group_line(around.others or {}, "creature", "creatures")
  if others then out[#out + 1] = others end

  local landmarks = group_line(around.landmarks or {}, "way out", "ways out")
  if landmarks then out[#out + 1] = landmarks end

  -- Only the area is not an answer to "what is around me", so the empty word is
  -- still owed when nothing else was found.
  if #out == 0 or (#out == 1 and area ~= "") then out[#out + 1] = "Nothing nearby." end
  return out
end

return surroundings
