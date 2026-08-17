-- Turning the overmap cursor into speech.
--
-- Pure: a table in, a list of strings out. No game state, no gapi, so every
-- decision here is assertable without a world.
--
-- The overmap is the one screen in the game whose whole subject is a place
-- rather than a thing, and it is what turns the world from squares into named
-- destinations: the game itself describes a tile as "house in central
-- Springfield", and its own travel key walks the character there. So the layer
-- speaks the screen rather than building a second way to navigate.
--
-- It is not a list, so the menu reading model does not fit it -- there is no
-- count and no position among entries. What carries over is its order: name
-- first (P2), then where it is, and one utterance per subject (F1). Where a
-- menu says "3 of 16" this says a distance and a compass point (P3), because
-- that is the only form of position a place on a map has for someone who cannot
-- look at it.
--
-- The hook fires once per input round, so the same tile arrives again after
-- every keypress it ignored. Saying only what changed is this module's job.

local text = require("./text")
local bearing = require("./bearing")

local overmap = {}

-- The screen's own key for previewing and then confirming travel. Named here
-- because the first press of it is silent by construction: it draws a path and
-- waits for a second press, so both "there is a route" and "there can be no
-- route" are answered by nothing at all without this module.
local TRAVEL = "CHOOSE_DESTINATION"

--- Normalise one firing of the hook, with the game's markup already stripped so
--- comparisons never see colour codes.
--- @param params table
--- @return table
overmap.state = function(params)
  return {
    -- What screen this state belongs to. Every screen in the layer shares one
    -- variable for whatever is on top, because whichever screen holds the
    -- keyboard is the only one that can hold it -- so a state from another
    -- screen arrives here on the way back, and has to read as an arrival rather
    -- than as a tile to compare against.
    screen = "overmap",
    place = text.clean(params.place),
    note = text.clean(params.note),
    seen = params.seen == true,
    explored = params.explored == true,
    dx = params.dx or 0,
    dy = params.dy or 0,
    dz = params.dz or 0,
    route = params.route or 0,
    action = params.action or "",
  }
end

--- Levels between the cursor and the character, as words.
---
--- Said even though the map is drawn one level at a time: a tile directly
--- underfoot and a tile three floors down read identically otherwise, and the
--- difference is a staircase hunt.
local function level_words(dz)
  if dz == 0 then return nil end
  local levels = math.abs(dz) == 1 and "1 level" or math.abs(dz) .. " levels"
  return levels .. (dz > 0 and " up" or " down")
end

--- Where the cursor is, relative to the character. Never a coordinate (P3).
local function where_words(state)
  local parts = {}
  local flat = bearing.describe(state.dx, state.dy)
  if flat then parts[#parts + 1] = flat end

  local levels = level_words(state.dz)
  if levels then parts[#parts + 1] = levels end

  -- The cursor starts on the character and returns there on the centring key,
  -- and that is worth a word: it is the one position on the map she can be sure
  -- of, and a bearing of zero would otherwise read as no answer at all.
  if #parts == 0 then return "here" end
  return table.concat(parts, ", ")
end

--- The tile, in the order P2 fixes: what it is, then where it is, then what is
--- known about it.
---
--- A tile that has never been seen says so instead of naming itself. That is not
--- a missing answer but the answer: the map is a memory, and the blank on it is
--- what a player without sight otherwise has no way to find the edge of.
local function tile_line(state)
  local parts = {}
  parts[#parts + 1] = (state.seen and text.is_speakable(state.place)) and state.place or "Unexplored"
  parts[#parts + 1] = where_words(state)
  -- Explored means she has already been through it and marked it off. It is
  -- drawn as a colour on the map and said nowhere.
  if state.explored then parts[#parts + 1] = "explored" end
  return table.concat(parts, ", ") .. "."
end

--- How long the previewed route is. A route is a number of overmap tiles, each
--- of which is a walk across a screen, so the figure is what says whether the
--- destination is an afternoon away or a week.
local function route_line(route)
  if route == 1 then return "Route: 1 tile." end
  return string.format("Route: %d tiles.", route)
end

--- Why the travel key answered with nothing.
---
--- The screen draws nothing in all three cases and the game refuses a path in
--- all three, so one word for them would be true and useless: "no route" sends
--- her looking for another way round when what is needed is to look at the place
--- first, or to notice that the cursor never left her.
local function refusal_words(state)
  if state.dx == 0 and state.dy == 0 and state.dz == 0 then return "You are here." end
  -- The game will not path into what the map does not hold yet, whatever lies
  -- there in fact. So this is the one refusal that is answered by going to look.
  if not state.seen then return "Not on the map yet." end
  return "No route."
end

--- What to say about this firing, given the one before it.
--- @param state table normalised by overmap.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
overmap.utterances = function(state, previous)
  local out = {}

  -- What the firing before this one said, and nothing at all when there was no
  -- such firing or when the state belongs to another screen: every screen in the
  -- layer shares one variable for whatever is on top, so a menu drawn over the
  -- map arrives here on the way back, and closing it is arriving.
  local before = (previous and previous.screen == "overmap") and previous or nil
  local was_line = before and tile_line(before) or nil
  local was_note = before and before.note or ""
  local was_route = before and before.route or 0

  -- Arriving is said once, because the screen takes the whole keyboard and
  -- nothing else in the game answers a key the way it does.
  local arrived = before == nil
  if arrived then out[#out + 1] = "Overmap." end

  local line = tile_line(state)
  local moved = arrived or line ~= was_line
  if moved then out[#out + 1] = line end

  -- Her own note, as its own utterance and after the tile it belongs to: the
  -- note is what she wrote about the place, not part of what the place is.
  if state.note ~= "" and (moved or state.note ~= was_note) then
    out[#out + 1] = string.format("Note: %s.", state.note)
  end

  if state.route > 0 then
    if moved or state.route ~= was_route then out[#out + 1] = route_line(state.route) end
  elseif not arrived and state.action == TRAVEL then
    -- The travel key answered, and there is no route to answer with. Which of
    -- the three reasons it is decides what she does next, so it is said rather
    -- than flattened into one refusal.
    out[#out + 1] = refusal_words(state)
  end

  return out
end

return overmap
