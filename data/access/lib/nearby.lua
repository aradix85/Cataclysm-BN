-- Turning the list of what is lying around into speech.
--
-- Pure: tables in, list of strings out. No game state, no gapi.
--
-- The game's own list of every item in view, on its own key. It is the answer to
-- "is there anything around here worth walking to", which the look-around cursor
-- can only give square by square -- the exhaustion F5 names. So it is read the way
-- every list in the layer is read, by menus.lua, with one thing added: where the
-- item is.
--
-- That addition is the whole point of the screen. It draws a trail across the map
-- to the selected row and prints no direction anywhere, so a name without a bearing
-- is a catalogue rather than a way to reach anything. It goes into the field
-- menus.lua already speaks beside a name -- the second column, short by
-- construction -- rather than into a sentence of its own, because what is there and
-- which way to walk answer one question.
--
-- The category heading the screen draws while it is sorted that way is handed over
-- and deliberately not spoken: this screen is asked where things are, and the
-- inventory is where what-group-is-this belongs.

local menus = require("./menus")
local bearing = require("./bearing")

local nearby = {}

--- Levels between the item and the character, as words. An item one floor down is
--- not two squares away however the bearing reads.
local function level_words(dz)
  if dz == 0 then return nil end
  local levels = math.abs(dz) == 1 and "1 level" or math.abs(dz) .. " levels"
  return levels .. (dz > 0 and " up" or " down")
end

--- Where the item is, relative to the character. Never a coordinate (P3).
local function place_words(entry)
  local parts = {}
  local flat = bearing.describe(entry.dx or 0, entry.dy or 0)
  if flat then parts[#parts + 1] = flat end

  local levels = level_words(entry.dz or 0)
  if levels then parts[#parts + 1] = levels end

  -- An item on her own square has no direction, and that is the answer: it is
  -- already at her feet.
  if #parts == 0 then return "here" end
  return table.concat(parts, ", ")
end

--- The screen as menus.lua wants it, with the place in the column it speaks.
--- @param params table
--- @return table
nearby.state = function(params)
  local entry = params.entry

  return menus.state({
    category = "LIST_ITEMS",
    title = "Items nearby",
    text = "",
    count = params.count,
    cursor = params.cursor,
    entry = entry and { text = entry.text, column = place_words(entry) } or nil,
  })
end

--- What to say about this firing, given the one before it.
---
--- Nothing of its own: the reading model answers the opening line, the count when a
--- filter changes it, and the row -- and the row now carries where to walk.
--- @param state table normalised by nearby.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
nearby.utterances = function(state, previous)
  return menus.utterances(state, previous)
end

return nearby
