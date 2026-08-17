-- Turning the inventory screens into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi.
--
-- This is the system that says nothing at all by itself: src/inventory_ui.cpp
-- has no message sites and no hooks of its own, so before the firing point in
-- src/inventory_hook.h a player without sight could open the inventory and hear
-- silence, whatever they pressed. Every screen built on inventory_selector
-- arrives here -- the plain inventory, wield, wear, eat, drop, pick up, use --
-- because all of them wait for a key in the same place.
--
-- It is a titled list with a position in it, so it is read by the model
-- menus.lua settles rather than a second one. What it adds is what an inventory
-- has and a menu does not: rows are grouped under headings the game draws and
-- never speaks, rows can be marked, and a row that cannot be chosen carries the
-- reason why.

local menus = require("./menus")
local text = require("./text")

local inventory = {}

--- Where the row's item is, in words, or nothing when the player carries it.
---
--- The screens that offer nearby items draw them in a column of their own, and
--- a column is a position on a screen: nothing at all without sight. What the
--- player has on them is already named by the headings the game writes --
--- "WEAPON HELD", "ITEMS WORN", and a plain heading for the pack -- so only the
--- places outside the character need a word, and the one that matters most is
--- the pile at their own feet, which the game names no differently from the
--- pack it would go into.
local PLACE = {
  map = "on the ground",
  vehicle = "in the vehicle",
  container = "in a container",
}

--- Compass points as the game abbreviates them in a heading.
local POINTS = {
  N = "north",
  S = "south",
  E = "east",
  W = "west",
  NE = "northeast",
  NW = "northwest",
  SE = "southeast",
  SW = "southwest",
}

--- A heading as the screen draws it, said as a word rather than shouted.
---
--- The game writes these in capitals -- "WEAPONS", "FOOD" -- because that is
--- how they are drawn, and a synthesiser given capitals either shouts them or
--- spells them out.
---
--- Items on another square carry how far and which way in the heading itself,
--- written as tightly as a screen needs it: "FOOD 3NE". Left as it is that
--- reads as a syllable rather than a direction, so it is spelt out into the
--- same words every other bearing in the layer uses (P3).
--- @param group string
--- @return string
local function spoken_group(group)
  local said = group:sub(1, 1):upper() .. group:sub(2):lower()
  local head, distance, point = said:match("^(.-)%s*(%d+)(%a+)$")
  if head == nil or distance == nil or point == nil then return said end
  local named = POINTS[point:upper()]
  if named == nil then return said end
  return head .. " " .. distance .. " " .. named
end

--- The one thing worth saying about the row besides its name.
---
--- Two answers to "what is different about this row", and only one of them is
--- ever true at a time: a row that cannot be chosen cannot be marked either. The
--- reason is the screen's own words -- "Too heavy to wield", "You have nothing
--- to reload" -- and without it a key that does nothing is indistinguishable
--- from a key that is broken.
--- @param entry table
--- @return string|nil
local function column_of(entry)
  if entry.marked and entry.marked > 0 then
    return entry.marked .. " marked"
  end
  if text.is_speakable(entry.denial or "") then return entry.denial end
  return nil
end

--- The screen as menus.lua wants it, plus the heading the selected row sits
--- under, which is this screen's alone.
--- @param params table
--- @return table
inventory.state = function(params)
  local entry = params.entry

  local state = menus.state({
    -- Every one of these screens shares one input context, so the title is what
    -- says which of them is open -- "Inventory", "Drop items", "Consume item".
    category = "INVENTORY",
    title = params.title,
    text = "",
    count = params.count,
    cursor = params.cursor,
    entry = entry and {
      text = entry.text,
      column = column_of(entry),
      enabled = entry.enabled,
    } or nil,
  })

  -- Kept apart from the fields menus.lua compares, which it ignores.
  state.group = entry and text.clean(entry.category) or ""
  state.place = entry and PLACE[entry.where or ""] or nil
  return state
end

--- Whether the heading above the selection is new, and therefore worth a word.
---
--- Said when it changes and never otherwise. An inventory is grouped, the
--- grouping is the only structure the screen has, and stepping from the last
--- weapon into the first food is otherwise silent -- so the player learns where
--- the list ends only by walking off it. Repeating it on every row instead
--- would be a second sentence per keypress about something that changes every
--- twenty (F5).
---
--- Where the items are is part of the same sentence rather than one of its own:
--- crossing from the pack into the pile at the player's feet is arriving
--- somewhere, exactly as crossing into another heading is, and it is the answer
--- to the only question a drop or pick-up screen really asks -- is this mine or
--- is this lying there.
--- @param state table
--- @param previous table|nil
--- @return string|nil
local function group_sentence(state, previous)
  if not state.entry or not text.is_speakable(state.group) then return nil end
  if
    previous
    and menus.same_menu(state, previous)
    and previous.group == state.group
    and previous.place == state.place
  then
    return nil
  end
  if state.place then return spoken_group(state.group) .. ", " .. state.place .. "." end
  return spoken_group(state.group) .. "."
end

--- What to say about this firing, given the one before it.
---
--- The heading goes directly before the row it heads, which is always the last
--- thing the reading model says: what the screen is, then where in it the cursor
--- has arrived, then the row itself. Never the list (F5) -- a full inventory
--- runs to hundreds of rows and the opening line says only how many.
--- @param state table normalised by inventory.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
inventory.utterances = function(state, previous)
  local out = menus.utterances(state, previous)
  if #out == 0 then return out end

  local group = group_sentence(state, previous)
  if group then table.insert(out, #out, group) end
  return out
end

return inventory
