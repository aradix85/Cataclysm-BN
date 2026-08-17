-- Turning the game's own detail screen into speech.
--
-- Pure: a table in, a list of strings out. No game state, no gapi.
--
-- The screen the describe key opens over a square, and the layer's first wall of
-- prose: a full description of the creature, the furniture or the terrain there,
-- with what can be harvested from it and what it deconstructs into. The screen
-- prints all of it at once and scrolls nowhere, so there is nothing to page through
-- and nothing to ask for a second time.
--
-- So it is read out, and in pieces. This is the one place in the layer where length
-- is the point rather than the enemy: she pressed a key whose whole purpose is
-- "tell me everything about this", and the answer is owed in full. What F5 asks for
-- here is not brevity but breaks -- one utterance per line the game wrote, so that
-- speech can be cut off at any of them and picked up nowhere else.
--
-- Nothing is said twice. The screen fires again after every key it ignored, and it
-- answers three keys of its own that swap which of the three things is described,
-- so what changed is the target.

local text = require("./text")

local describe = {}

-- What each of the three is called, and what it means when there is nothing of that
-- kind on the square. The screen has a sentence for each of those three cases and
-- they all say the same thing, so one wording answers all of them.
local TARGETS = {
  creature = "Creature",
  furniture = "Furniture",
  terrain = "Terrain",
}

--- Normalise one firing.
--- @param params table
--- @return table
describe.state = function(params)
  local target = params.target or "terrain"
  return {
    -- Which screen this state belongs to; every screen in the layer shares one
    -- variable for whatever is on top, so a state from another screen arrives here
    -- on the way back and has to read as an arrival.
    screen = "describe",
    target = target,
    lines = text.lines(params.text),
    signage = text.clean(params.signage),
  }
end

--- What to say about this firing, given the one before it.
--- @param state table normalised by describe.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
describe.utterances = function(state, previous)
  local before = (previous and previous.screen == "describe") and previous or nil
  if before and before.target == state.target then return {} end

  local out = {}
  local name = TARGETS[state.target] or "Terrain"

  if #state.lines == 0 then
    -- Nothing of that kind is here, which is an answer and not a failure: she
    -- pressed the key for creatures on an empty square, or is looking at a square
    -- she cannot see into.
    out[#out + 1] = string.format("No %s here.", name:lower())
  else
    -- The heading says which of the three keys is answering, since all three are
    -- one keypress apart and the description alone does not say which was pressed.
    out[#out + 1] = name .. "."
    for _, line in ipairs(state.lines) do
      out[#out + 1] = line
    end
  end

  -- What is written on the square, which the screen appends whatever the target is,
  -- and which is the one thing here that is not a description at all.
  if text.is_speakable(state.signage) then
    out[#out + 1] = string.format("Sign: %s.", state.signage)
  end

  return out
end

return describe
