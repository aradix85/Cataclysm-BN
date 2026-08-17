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

--- What to say about the surroundings.
---
--- Answering "nothing nearby" rather than staying silent is deliberate: P5 is
--- about not volunteering emptiness, and this was asked for. A silent answer to
--- a keypress cannot be told apart from a key that never arrived.
---
--- The area comes first because it is the largest thing true of where she is
--- standing, and because it is the answer to a question that otherwise costs a
--- trip to the overmap and back: the game names the region a square belongs to
--- and never says it out loud anywhere in play. The room's reach follows it, since
--- the two together are where she is; everything after them is what is in it.
--- @param around table { area, reach, enemies, others, landmarks }
--- @return string[]
surroundings.overview = function(around)
  local out = {}

  local area = around.area or ""
  if area ~= "" then out[#out + 1] = area:sub(1, 1):upper() .. area:sub(2) .. "." end

  local reach = reach_line(around.reach)
  if reach then out[#out + 1] = reach end

  local enemies = group_line(around.enemies or {}, "enemy", "enemies")
  if enemies then out[#out + 1] = enemies end

  local others = group_line(around.others or {}, "creature", "creatures")
  if others then out[#out + 1] = others end

  local landmarks = group_line(around.landmarks or {}, "way out", "ways out")
  if landmarks then out[#out + 1] = landmarks end

  -- Stairs last, and apart from the doors: a door leads to the next room and a
  -- staircase leads out of this place entirely, usually into somewhere with
  -- another name. Which way they go is the whole of what makes them worth
  -- walking to, so up and down are never one group.
  local up = group_line(around.ways_up or {}, "way up", "ways up")
  if up then out[#out + 1] = up end

  local down = group_line(around.ways_down or {}, "way down", "ways down")
  if down then out[#out + 1] = down end

  -- Where she is is not an answer to "what is around me", so the empty word is
  -- still owed when nothing else was found.
  local placed = (area ~= "" and 1 or 0) + (reach and 1 or 0)
  if #out == placed then out[#out + 1] = "Nothing nearby." end
  return out
end

return surroundings
