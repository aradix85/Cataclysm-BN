-- Turning the ways out of a place into a list she can walk through.
--
-- Pure: tables in, list of strings out. No game state, no gapi.
--
-- This is the first screen the layer owns rather than speaks. Everywhere else it
-- listens to a screen the game already draws, and that is the rule -- the game's
-- own screens keep their keys and their order, so the wiki and other players keep
-- describing the same game she is playing. Here there is nothing to listen to: no
-- screen in the game answers "which ways lead out of this building, and where are
-- they", because a sighted player reads that off the map in one glance.
--
-- Why a list rather than the sentence F9 already gives. A sentence goes past once
-- and takes the nearest thing with it; the second and third door are never heard,
-- and nothing can be gone back to. A list is walked at her own pace, and a row can
-- be sat on until it is remembered. For building a picture of a place that is a
-- different instrument, not a longer one.
--
-- It is read by the model every list in the layer uses -- menus.lua -- so it is one
-- more list rather than a second way of reading.

local menus = require("./menus")
local bearing = require("./bearing")

local exits = {}

-- What each kind is called when it is said. The name of the terrain says what it
-- is -- "wooden door", "stairs" -- and this says what it does, which is the part
-- that decides whether it is worth crossing a room for.
local KINDS = {
  up = "up",
  down = "down",
}

--- Where an exit is, relative to her. Never a coordinate (P3).
local function place_words(row)
  local flat = bearing.describe(row.dx or 0, row.dy or 0)
  return flat or "here"
end

--- One row as the reading model wants it: what it is and which way it goes in the
--- name, where it is in the column the model already speaks beside a name.
local function entry_of(row)
  local kind = KINDS[row.kind]
  local name = row.name or ""
  return { text = kind and (name .. " " .. kind) or name, column = place_words(row) }
end

--- The screen as menus.lua wants it.
--- @param list table[] rows as perception.exits_in_zone returns them
--- @param cursor integer|nil the 1-based row the selection is on
--- @param place string|nil the name of the place these are the ways out of
--- @return table
exits.state = function(list, cursor, place)
  local rows = list or {}
  local row = cursor and rows[cursor] or nil
  local title = place and place ~= "" and place or "Here"
  title = title:sub(1, 1):upper() .. title:sub(2)

  return menus.state({
    category = "BN_ACCESS_EXITS",
    title = title .. ", ways out",
    text = "",
    count = #rows,
    cursor = row and cursor or nil,
    entry = row and entry_of(row) or nil,
  })
end

--- What to say about this state, given the one before it.
--- @param state table normalised by exits.state
--- @param previous table|nil
--- @return string[]
exits.utterances = function(state, previous)
  return menus.utterances(state, previous)
end

return exits
