-- Turning the menu the player is in into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi, so
-- every decision here is assertable without a game and without NVDA.
--
-- Every uilist shares one input context, so this one module reaches the menu
-- ESC opens, the action menu on RETURN, the item action menus, the examine
-- menus, the vehicle controls and the debug menu. It is also the first list the
-- layer builds, and the reading model settled here is the one every later list
-- reuses: an opening line naming the menu and its size, then one entry per
-- keypress, name first, and a word where the list wraps instead of a silent
-- jump back to the other end.
--
-- The hook fires once per input round, not once per menu, so the same menu
-- arrives again after every keypress, answered or not. Working out what
-- changed is this module's whole job: repeating the title on every arrow key
-- is F1 and F5 at once, and saying nothing when the selection moved is F2, the
-- failure that cost a blind CDDA player playability outright.

local text = require("./text")

local menus = {}

-- A description is a level of detail, not part of a list (P1), and there is no
-- key inside a uilist to ask for one with: a menu accepts its own keys and
-- swallows every other one. So `desc` is deliberately never spoken here, and
-- the day it is asked for it needs a way to be asked for.
--
-- The second column is spoken, because it is drawn on the same row as the name,
-- is short by construction -- "Level 3", "Can't learn!", a hit point figure --
-- and is often the only thing telling two identically named entries apart.

--- Normalise one firing of the hook into a plain table, with everything the
--- game draws already stripped so that comparisons never see markup.
--- @param params table
--- @return table
menus.state = function(params)
  local entry = params.entry
  return {
    category = params.category,
    title = text.clean(params.title),
    text = text.clean(params.text),
    count = params.count or 0,
    cursor = params.cursor,
    entry = entry
        and {
          text = text.clean(entry.text),
          column = text.clean(entry.column),
          -- Absent means enabled: the game sets this on every entry, and a test
          -- that leaves it out is describing an ordinary one.
          enabled = entry.enabled ~= false,
        }
      or nil,
  }
end

--- Whether two firings describe the same menu still waiting for a key.
---
--- The count is not part of what identifies a menu: filtering changes it while
--- the menu stays the same one, and that change is worth a word of its own.
local function same_menu(state, previous)
  if not previous then return false end
  return state.category == previous.category and state.title == previous.title and state.text == previous.text
end

--- The entry's name, and whether choosing it descends into another menu.
---
--- An entry that opens a further menu is drawn with a trailing ellipsis, and
--- that is the only sign the game gives that RETURN will not act but descend.
--- A synthesiser says nothing at all for it, so the mark is turned into a word
--- rather than lost. Both spellings occur: the character and three full stops.
local function name_of(entry)
  local trimmed = entry.text:gsub("%s*%.%.%.$", ""):gsub("%s*\226\128\166$", "")
  return trimmed, trimmed ~= entry.text
end

--- How many entries the filter leaves, as words rather than a bare number.
local function count_words(count)
  if count == 0 then return "no entries" end
  if count == 1 then return "1 entry" end
  return count .. " entries"
end

--- The same, as a sentence of its own, for when only the count changed.
local function count_sentence(count)
  local words = count_words(count)
  return words:sub(1, 1):upper() .. words:sub(2) .. "."
end

--- What the menu is, said once when it opens.
---
--- Both the title in the border and the header above the list are taken, since
--- a menu may carry either: the menu ESC opens has no title and calls itself
--- "MAIN MENU" in the header, while others do the reverse. Neither is repeated
--- afterwards -- a menu that names itself on every keypress is unusable.
local function heading(state)
  local parts = {}
  if text.is_speakable(state.title) then parts[#parts + 1] = state.title end
  if text.is_speakable(state.text) and state.text ~= state.title then parts[#parts + 1] = state.text end
  local name = #parts > 0 and table.concat(parts, ". ") or "Menu"
  return name .. ", " .. count_words(state.count) .. "."
end

--- The selected entry, in the order P2 fixes: name first, so the player can
--- stop listening as soon as they know enough, then where it sits, then what is
--- wrong with it. Nil when the filter leaves nothing selected.
local function entry_line(state)
  local entry = state.entry
  if not entry then return nil end

  local name, opens = name_of(entry)
  local parts = {}
  -- A menu draws blank entries as separators. The cursor normally steps over
  -- them, but a menu that highlights disabled entries can land on one, and a
  -- keypress that answers with silence is indistinguishable from a dead key.
  parts[#parts + 1] = text.is_speakable(name) and name or "Blank"
  if opens then parts[#parts + 1] = "submenu" end
  if text.is_speakable(entry.column) then parts[#parts + 1] = entry.column end
  if state.cursor then parts[#parts + 1] = state.cursor .. " of " .. state.count end
  if not entry.enabled then parts[#parts + 1] = "unavailable" end

  return table.concat(parts, ", ") .. "."
end

--- Whether the selection jumped from one end of the list to the other.
---
--- Stepping one past the last entry lands on the first, and one before the
--- first lands on the last (src/ui.cpp, uilist::scrollby). Without a word there
--- the list simply appears to go on forever. A changed count means the filter
--- moved the selection rather than the player, so it is not a wrap.
---
--- With two entries nothing is claimed, because every move in such a list is
--- between its two ends: stepping down from the first onto the last and wrapping
--- up onto it are the same two numbers. The position already says where the
--- cursor landed, and a wrap announced when none happened sends a player back
--- up a list they were walking down.
local function wrap_word(state, previous)
  if state.count ~= previous.count or state.count < 3 then return nil end
  if not state.cursor or not previous.cursor then return nil end
  if previous.cursor == state.count and state.cursor == 1 then return "Back to the top." end
  if previous.cursor == 1 and state.cursor == state.count then return "Back to the bottom." end
  return nil
end

--- What to say about this firing, given the one before it.
---
--- One utterance, one subject (F1): what the menu is, and what is selected in
--- it, are never the same sentence. Never the list (F5): a menu can hold
--- hundreds of entries and the opening line says only how many.
---
--- Within the same menu the entry is spoken whenever it reads differently,
--- which covers both the selection moving and an entry rewriting itself where
--- it stands -- a toggle that answers RETURN by changing its own text would
--- otherwise be answered by silence.
--- @param state table normalised by menus.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
menus.utterances = function(state, previous)
  local out = {}

  if previous and same_menu(state, previous) then
    -- The filter moved. How much is left is the answer to typing, and it also
    -- says that a search found nothing without the player having to walk it.
    if state.count ~= previous.count then out[#out + 1] = count_sentence(state.count) end
    local wrapped = wrap_word(state, previous)
    if wrapped then out[#out + 1] = wrapped end

    local line = entry_line(state)
    if line and line ~= entry_line(previous) then out[#out + 1] = line end

    -- Anything else is the same menu redrawn, and silence is what keeps it
    -- from talking over itself on every keypress it ignored (P5).
    return out
  end

  out[#out + 1] = heading(state)
  local line = entry_line(state)
  if line then out[#out + 1] = line end
  return out
end

return menus
