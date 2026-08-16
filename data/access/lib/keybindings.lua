-- Turning the game's own keybindings screen into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi.
--
-- That screen is reached from 81 input contexts on the question mark, and it is
-- the only place in the game that says what a screen can do. It is also where a
-- key is changed, which matters more here than for a sighted player: the
-- defaults were chosen for a keyboard with a numpad, and the owner's has none.
--
-- It is not a uilist, so it arrives through a hook of its own -- see
-- src/keybindings_hook.h -- but it is a list like any other and the reading
-- model is the one menus.lua settles. Everything here is composition of that; a
-- second reading model would be a bug.
--
-- What is different is that reading the list is not enough to use it. Choosing
-- a row means pressing a letter that the screen only draws while it is in one
-- of its change modes, and those modes are started by three keys named only in
-- a legend nobody hears. Both are said here, and nowhere else in the layer.

local menus = require("./menus")

local keybindings = {}

--- Where a key works, in words rather than in the colour the screen uses.
---
--- Green for a key that works only on the screen being described, grey for one
--- that works across the game -- and colour is nothing at all to a player who
--- cannot see it. It is also exactly the difference between the two keys that
--- add a binding, so without this a change cannot be made deliberately.
---
--- Absent where the action has no key: the second column already says so.
local SCOPE = {
  ["local"] = "this screen only",
  global = "everywhere",
}

--- The screen as menus.lua wants it, plus what belongs to this screen alone.
---
--- The second column carries the keys bound to the action and where they work
--- -- or, while letters are picking rows, the letter that picks this one. Two
--- different answers to "what is this row's key", and only one of them is true
--- at a time: outside a change mode no letter is drawn, and inside one the
--- bound key is not what the next keypress is about.
--- @param params table
--- @return table
keybindings.state = function(params)
  local entry = params.entry
  local picking = params.picking == true

  local column = nil
  if entry then
    column = entry.column
    local scope = entry.scope and SCOPE[entry.scope]
    if scope then column = column .. ", " .. scope end
    if picking then column = entry.letter end
  end

  local state = menus.state({
    category = params.category,
    title = params.title,
    text = "",
    count = params.count,
    cursor = params.cursor,
    entry = entry and { text = entry.text, column = column } or nil,
  })

  state.picking = picking
  state.keys = params.keys or {}
  -- Kept apart from the menu's own fields, which menus.lua compares to work out
  -- what changed; these are extra and it ignores them.
  return state
end

--- How a key is changed here, in the screen's own three keys.
---
--- Said once when the screen opens and never again, and last rather than first:
--- what the player asked for is the list, and an instruction read before the
--- content is F4. Whether it is worth hearing at all is a question for her ears
--- -- the alternative is that nobody who has not read a manual can ever change
--- a key.
local function how_to_change(keys)
  if not keys.add_local or keys.add_local == "" then return nil end
  return "Press "
    .. keys.add_local
    .. " to add a key here, "
    .. keys.add_global
    .. " to add it everywhere, "
    .. keys.remove
    .. " to remove one."
end

--- What to say about this firing, given the one before it.
---
--- The row itself carries the change: entering a mode rewrites its second
--- column from the bound key to the letter, so menus.lua hears a line that
--- reads differently and says it. That is the whole announcement of the mode,
--- and it is better than a sentence about the mode would be, because it answers
--- with the thing the player has to press.
--- @param state table normalised by keybindings.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
keybindings.utterances = function(state, previous)
  local out = menus.utterances(state, previous)

  if not menus.same_menu(state, previous) then
    local how = how_to_change(state.keys)
    if how then out[#out + 1] = how end
  end

  return out
end

return keybindings
