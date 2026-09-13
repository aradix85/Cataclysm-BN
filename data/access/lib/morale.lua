-- Turning the game's own morale screen into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi.
--
-- That screen opens on a key in ordinary play and is the game's own answer to
-- "how am I feeling and why". It is not a uilist, so it arrives through a hook of
-- its own -- see src/morale_hook.h -- but it is a titled list of rows and the
-- reading model is the one menus.lua settles. Everything here is composition of
-- that; a second reading model would be a bug.
--
-- What is different is that the reading position is ours. The screen has no
-- selection at all: it scrolls a window, and only when the list does not fit,
-- which it usually does -- so every arrow key is answered by nothing and no row
-- below the first can be reached. On the keybindings screen that had to be
-- repaired in the screen itself, because a row there is chosen and the screen has
-- to agree about which one. Here no row can be acted on, so the cursor is a place
-- in a reading and nothing else, and it lives here where a test can drive it.
--
-- The total is said when the screen opens rather than waited for at the end of the
-- list, because it is what the whole screen is about. What the screen draws below
-- the list -- pain, the cap fatigue puts on morale, where focus is heading -- is
-- said once after the first row, last rather than first: what was asked for is the
-- list, and an instruction read before the content is F4.

local menus = require("./menus")
local text = require("./text")

local morale = {}

--- Where the reading is after this keypress.
---
--- Wraps at both ends, as every list in the layer does: a step that moves nothing
--- is indistinguishable from a dead keyboard. Any other key leaves the cursor
--- where it was, which is how a key the screen ignored stays silent -- the row
--- reads the same, so menus.lua says nothing.
--- @param cursor integer|nil where the reading was
--- @param action string|nil what the screen's previous round answered
--- @param count integer how many rows there are now
--- @return integer|nil
local function step(cursor, action, count)
  if count == 0 then return nil end
  local at = cursor or 1

  if action == "DOWN" then
    return at < count and at + 1 or 1
  elseif action == "UP" then
    return at > 1 and at - 1 or count
  end
  return math.min(at, count)
end

--- The screen as menus.lua wants it, plus what belongs to this screen alone.
---
--- The second column carries the figure the screen draws beside the row: a share
--- of the whole for a source, a signed total for a group.
--- @param params table
--- @param previous table|nil the state of the firing before this one
--- @return table
morale.state = function(params, previous)
  local rows = params.rows or {}
  local cursor = nil

  if previous and previous.screen == "morale" then
    cursor = step(previous.cursor, params.action, #rows)
  elseif #rows > 0 then
    -- Arriving: the reading starts at the top, which is the first group's total.
    cursor = 1
  end

  local entry = cursor and rows[cursor] or nil
  local state = menus.state({
    -- The screen's own input context. Nothing else in the game uses it, so it is
    -- what tells a state of this screen from a state of another.
    category = "MORALE",
    title = params.title,
    -- What the whole screen is about, said in the opening line rather than left
    -- at the bottom of a list she would have to walk to the end of.
    text = "Total morale " .. (params.total or ""),
    count = #rows,
    cursor = cursor,
    entry = entry and { text = entry.text, column = entry.value } or nil,
  })

  -- Kept apart from the menu's own fields, which menus.lua compares to work out
  -- what changed; these are extra and it ignores them.
  state.screen = "morale"
  state.summary = params.summary or {}
  return state
end

--- The figures the screen draws below the list, as one sentence.
---
--- One sentence and not one utterance each: each is two words and a number, and
--- three of them in a row is a burst of starts and stops rather than an answer.
--- @param summary table[]
--- @return string|nil
local function summary_sentence(summary)
  local parts = {}
  for _, row in ipairs(summary) do
    local label = text.clean(row.text)
    if text.is_speakable(label) then parts[#parts + 1] = label .. " " .. text.clean(row.value) end
  end
  if #parts == 0 then return nil end
  return table.concat(parts, ", ") .. "."
end

--- What to say about this firing, given the one before it.
---
--- The reading model answers the opening line and the row. The summary is said
--- only on arrival, because none of it changes while the screen is open: the
--- screen works its figures out once, before it draws anything.
--- @param state table normalised by morale.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
morale.utterances = function(state, previous)
  local out = menus.utterances(state, previous)

  if not menus.same_menu(state, previous) then
    local line = summary_sentence(state.summary)
    if line then out[#out + 1] = line end
  end

  return out
end

return morale
