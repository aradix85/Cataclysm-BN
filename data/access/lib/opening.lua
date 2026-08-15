-- Turning the screen the game opens on into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi.
--
-- This screen is not a uilist, so it arrives through a hook of its own -- see
-- src/main_menu_hook.h -- but it is a list like any other and the reading model
-- is the one menus.lua already settles: an opening line naming the screen and
-- its size, then one entry per keypress, name first, and a word where the list
-- wraps. Everything here is composition of that; a second reading model would
-- be a bug.
--
-- What is different is that the screen holds two lists at once. The row of
-- headings along the top is one, and the list drawn under whichever heading is
-- selected is another. Both move under the arrow keys -- left and right along
-- the top, up and down inside -- so which of the two moved is the thing that
-- has to be clear, and it is made clear by never saying more than what moved.

local menus = require("./menus")

local opening = {}

-- What the screen is called. The game draws its logo here and no name at all,
-- so this is the layer's word rather than the game's.
local SCREEN = "Main menu"

--- One level of the screen, as the hook hands it over.
---
--- The key is lowered. The game highlights whichever case it wrote first, so the
--- row reads "N" for New Game and "a" for Load, and a synthesiser answers a
--- capital with a pitch change or the word "cap" -- a difference that means
--- nothing here, since both cases open the same heading.
local function level(l)
  if not l then return nil end
  return {
    text = l.text or "",
    key = (l.key or ""):lower(),
    saves = l.saves,
    cursor = l.cursor,
    count = l.count or 0,
  }
end

--- Normalise one firing of the hook into a plain table.
---
--- The heading is always there; the list under it is not, since Help and Quit
--- carry none and MOTD and Credits show a page of text instead.
--- @param params table
--- @return table
opening.state = function(params)
  return { heading = level(params.heading), entry = level(params.entry) }
end

--- The row of headings, read as a list: the screen itself is the menu, and the
--- selected heading is the entry standing in it.
---
--- A heading is called a submenu whenever something is selected under it, which
--- is the one thing the row does not say for itself: on arrival the cursor is
--- already down in that list rather than waiting above it, so a player who hears
--- only the heading would step past its first line without knowing it was there.
local function headings_of(state)
  return menus.state({
    category = "MAIN_MENU",
    title = "",
    text = SCREEN,
    count = state.heading.count,
    cursor = state.heading.cursor,
    entry = {
      text = state.heading.text,
      column = state.heading.key,
      opens = state.entry ~= nil,
      enabled = true,
    },
  })
end

--- What is worth knowing about an entry besides its name.
---
--- A world carries how many characters live in it, and the game draws that as a
--- bracketed number after the name. It is said in words instead: nought there is
--- not decoration but the difference between a world that can be entered and one
--- that answers Return with "no characters to load", and Load is where a player
--- is looking for exactly that.
---
--- Everything else carries the letter that jumps to it, and never both.
local function aside(entry)
  if entry.saves == 0 then return "no characters" end
  if entry.saves == 1 then return "1 character" end
  if entry.saves then return entry.saves .. " characters" end
  return entry.key
end

--- The list under the selected heading, read as a list of its own and named
--- after the heading it hangs from.
local function entries_of(state)
  return menus.state({
    category = "MAIN_MENU",
    title = "",
    text = state.heading.text,
    count = state.entry.count,
    cursor = state.entry.cursor,
    entry = { text = state.entry.text, column = aside(state.entry), enabled = true },
  })
end

--- The same list, as it stood before anything in it was selected.
---
--- Stepping onto a heading names it, and that name is also the name of the list
--- underneath. So the level below is entered as a list whose name has just been
--- given: what is new there is the entry, and saying the name twice in two
--- breaths is exactly the interleaving F1 is about.
local function named_already(list)
  return menus.state({
    category = list.category,
    title = list.title,
    text = list.text,
    count = list.count,
  })
end

--- What to say about this firing, given the one before it.
---
--- Both levels are handed to the same reading model, in the order they are
--- drawn: the heading first, because it is what the level below belongs to.
--- Each says nothing when nothing about it changed, so an arrow key along the
--- top is answered by the heading and its first entry, an arrow key inside the
--- list by that entry alone, and a redraw by silence.
--- @param state table normalised by opening.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
opening.utterances = function(state, previous)
  local out = {}

  local headings = headings_of(state)
  for _, line in ipairs(menus.utterances(headings, previous and headings_of(previous))) do
    out[#out + 1] = line
  end

  if not state.entry then return out end

  local entries = entries_of(state)
  local before = named_already(entries)
  -- Only a list that was already open has a previous selection worth comparing
  -- against; a heading that just moved brought a different list with it.
  if previous and previous.entry and previous.heading.cursor == state.heading.cursor then
    before = entries_of(previous)
  end

  for _, line in ipairs(menus.utterances(entries, before)) do
    out[#out + 1] = line
  end

  return out
end

return opening
