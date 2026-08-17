-- Turning the look-around cursor into speech.
--
-- Pure: a table in, a list of strings out. No game state, no gapi.
--
-- This is the screen where saying everything is the failure. The game's own panel
-- prints terrain, furniture, fields, traps, the creature, the vehicle, every item
-- lying there and any graffiti, all at once, for every square the cursor touches.
-- Read out in full, per keypress, that is F5 exactly: complete and unusable. So
-- one square is one short sentence -- what is there and where it is -- and the
-- rest is a count or a word, with the game's own detail screen a keypress away.
--
-- Never a scan, either. Walking the cursor over ten squares to find a door is the
-- exhaustion this project exists to avoid, and F9 already answers "where are the
-- ways out" in one line. The cursor is for when a particular square matters.
--
-- The hook fires once per input round, so the same square arrives again after
-- every keypress it ignored, and saying only what changed is this module's job.

local text = require("./text")
local bearing = require("./bearing")

local look = {}

-- What the character can make out, in the words the game's own panel uses. The
-- panel writes them into a window and returns nothing, so the token comes from
-- the firing point and the wording is here.
local SIGHT_WORDS = {
  lit = "Bright light",
  dark = "Darkness",
  blur = "A bright pink blur",
  blur_dark = "A pink blur",
  hidden = "Unseen",
}

--- Normalise one firing, with everything the game draws already stripped.
--- @param params table
--- @return table
look.state = function(params)
  return {
    -- Which screen this state belongs to; every screen in the layer shares one
    -- variable for whatever is on top, so a state from another screen arrives
    -- here on the way back and has to read as an arrival.
    screen = "look",
    sight = params.sight or "hidden",
    -- The square itself: furniture where there is any, terrain otherwise, which
    -- is what the panel names first.
    name = text.clean(params.name),
    -- The region the square belongs to, as the panel prints it above the square.
    area = text.clean(params.area),
    -- The creature standing there, and how it is sensed when it cannot be seen:
    -- infrared and the special senses reach through darkness and through walls,
    -- and a layer that left them out would make a working mechanic non-existent.
    creature = text.clean(params.creature),
    sensed = params.sensed or {},
    sound = text.clean(params.sound),
    items = params.items or 0,
    dx = params.dx or 0,
    dy = params.dy or 0,
    dz = params.dz or 0,
    peeking = params.peeking == true,
  }
end

--- Levels between the cursor and the character, as words.
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

  -- The cursor starts on her and returns there on the centring key. Said as her
  -- own square rather than as a bare "here", because that is what it is: the game
  -- counts her as the creature standing there and the square is the one place on
  -- the map she can be sure of. A bearing of zero would otherwise read as no
  -- answer at all.
  if #parts == 0 then return "you are here" end
  return table.concat(parts, ", ")
end

--- What is on the square, name first (P2).
---
--- A creature comes before the ground it stands on, because it is the only thing
--- on a square that can act. A square that cannot be made out says so instead of
--- naming what is there: the panel does the same, and speaking the contents of a
--- square the character cannot see would be reporting what she cannot know.
local function what_words(state)
  if state.sight ~= "clear" then
    return SIGHT_WORDS[state.sight] or SIGHT_WORDS.hidden
  end

  local ground = text.is_speakable(state.name) and state.name or "nothing"
  if text.is_speakable(state.creature) then
    return string.format("%s on %s", state.creature, ground)
  end
  return ground
end

--- The square as one sentence: what, then where.
local function square_line(state)
  return string.format("%s, %s.", what_words(state), where_words(state))
end

--- How much is lying there, as a count rather than a list (P1). The game's own
--- list of everything in view is a keypress away on its own key, and reading a
--- pile out per cursor step is the flood this screen invites.
---
--- Nothing is counted on a square that cannot be made out. The panel lists items
--- only where sight is clear, and saying how many lie in the dark would be
--- reporting what the character has no way of knowing.
local function items_line(state)
  if state.sight ~= "clear" or state.items == 0 then return nil end
  if state.items == 1 then return "1 item." end
  return string.format("%d items.", state.items)
end

--- What to say about this firing, given the one before it.
--- @param state table normalised by look.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
look.utterances = function(state, previous)
  local out = {}

  local before = (previous and previous.screen == "look") and previous or nil
  local arrived = before == nil
  -- Everything compared against the firing before this one, read once here so
  -- that nothing below has to ask whether there was one.
  local was_area = before and before.area or ""
  local was_line = before and square_line(before) or nil
  local was_items = before and before.items or 0
  local was_sound = before and before.sound or ""

  if arrived then
    -- Peeking is a different act from looking: it leans around a corner or
    -- through a window, and what it shows is a place the character is not
    -- standing in.
    out[#out + 1] = state.peeking and "Peeking." or "Look around."
  end

  -- The region, said on arrival and whenever the cursor crosses out of it, which
  -- is where the panel prints it too.
  if state.area ~= "" and (arrived or state.area ~= was_area) then
    out[#out + 1] = state.area:sub(1, 1):upper() .. state.area:sub(2) .. "."
  end

  local line = square_line(state)
  local moved = arrived or line ~= was_line
  if moved then out[#out + 1] = line end

  -- How a creature that cannot be seen is sensed, in the game's own words. Said
  -- with the square, since without it the answer above is only "Darkness".
  if moved or arrived then
    for _, sensed in ipairs(state.sensed) do
      if text.is_speakable(sensed) then out[#out + 1] = text.clean(sensed) .. "." end
    end
  end

  local items = items_line(state)
  if items and (moved or state.items ~= was_items) then out[#out + 1] = items end

  -- Sound reaches through walls and at any visibility, which makes it the one
  -- channel that answers about a square nothing else can.
  if state.sound ~= "" and (moved or state.sound ~= was_sound) then
    out[#out + 1] = string.format("Sound: %s.", state.sound)
  end

  return out
end

return look
