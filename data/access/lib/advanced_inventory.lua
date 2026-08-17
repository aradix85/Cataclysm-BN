-- Turning the advanced inventory into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi.
--
-- The screen items are moved on: two panes side by side, one holding the cursor
-- and one receiving. It is the answer to the exhaustion of moving a pile one
-- item at a time by walking to it, and it is drawn entirely as position and
-- colour -- which pane is active, which square each pane is aimed at, which
-- square a row came from. None of that is text, so none of it exists without
-- sight.
--
-- Not lib/inventory.lua, though both are lists of items: that module reads one
-- pane of one screen, and everything here is about the pair. Both read by the
-- model menus.lua settles, which is what keeps one reading model.

local menus = require("./menus")
local text = require("./text")

local advanced_inventory = {}

--- One of the two places, as one phrase.
---
--- The vehicle comes first because it is the thing being opened, and the
--- direction stays because it is the half that says where to walk: a pane
--- aimed at a vehicle's cargo is still aimed at a square the player has to
--- stand next to.
--- @param place table|nil
--- @return string
local function place_of(place)
  if not place then return "" end
  local area = text.clean(place.area)
  local vehicle = text.clean(place.vehicle or "")
  if not text.is_speakable(vehicle) then return area end
  return vehicle .. " at " .. area
end

--- A heading as the screen draws it, said as a word rather than shouted.
---
--- Item categories are written in capitals -- "WEAPONS", "FOOD" -- because that
--- is how they are drawn, and a synthesiser given capitals either shouts them
--- or spells them out.
--- @param group string
--- @return string
local function spoken_group(group)
  return group:sub(1, 1):upper() .. group:sub(2):lower()
end

--- The screen as menus.lua wants it, plus the two fields that are this
--- screen's alone.
---
--- Both places go into the title, which is what menus.lua compares to decide
--- whether the screen is still the same one. That is exactly right here:
--- re-aiming either pane, and swapping which of them holds the cursor, all
--- leave the player somewhere else with another list in front of them, and
--- arriving somewhere is what an opening line is for.
--- @param params table
--- @return table
advanced_inventory.state = function(params)
  local entry = params.entry
  local source = place_of(params.source)
  local destination = place_of(params.destination)

  local state = menus.state({
    category = "ADVANCED_INVENTORY",
    title = "From " .. source .. " to " .. destination,
    text = "",
    count = params.count,
    cursor = params.cursor,
    entry = entry and { text = entry.text } or nil,
  })

  -- Kept apart from the fields menus.lua compares, which it ignores.
  state.group = entry and text.clean(entry.category or "") or ""
  state.square = entry and text.clean(entry.square or "") or ""
  return state
end

--- Where the selection now is within the pane, said when it changes.
---
--- Two things can change under a moving cursor and both are structure the
--- screen draws and never says: the category heading, which exists only while
--- the pane is sorted by category, and the square the row's item is lying on,
--- which differs per row only while the pane is showing everything around the
--- player at once. Either alone is a sentence; together they are one sentence,
--- because they answer the same question -- what am I in now.
--- @param state table
--- @param previous table|nil
--- @return string|nil
local function place_sentence(state, previous)
  if not state.entry then return nil end

  local parts = {}
  if text.is_speakable(state.group) then parts[#parts + 1] = spoken_group(state.group) end
  if text.is_speakable(state.square) then parts[#parts + 1] = state.square end
  if #parts == 0 then return nil end

  if
    previous
    and menus.same_menu(state, previous)
    and previous.group == state.group
    and previous.square == state.square
  then
    return nil
  end
  return table.concat(parts, ", ") .. "."
end

--- What to say about this firing, given the one before it.
---
--- The order is the reading model's: what the screen is and how much is in it,
--- then where in it the cursor has arrived, then the row itself. Never the list
--- (F5) -- a pane aimed at everything around the player runs to hundreds of
--- rows and the opening line says only how many.
--- @param state table normalised by advanced_inventory.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
advanced_inventory.utterances = function(state, previous)
  local out = menus.utterances(state, previous)
  if #out == 0 then return out end

  local place = place_sentence(state, previous)
  if place then table.insert(out, #out, place) end
  return out
end

return advanced_inventory
