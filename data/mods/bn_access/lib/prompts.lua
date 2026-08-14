-- Turning a blocking prompt into speech.
--
-- Pure: tables in, list of strings out. No game state, no speech, no gapi, so
-- every decision here is testable without a game and without NVDA.
--
-- The hook fires once per input round, not once per popup, so the same prompt
-- arrives again after every keypress that did not answer it. Working out what
-- actually changed is this module's whole job: repeating the question on every
-- arrow key is F1 and F5 at once, and saying nothing when the selection moved
-- is F2, the failure that cost a blind CDDA player playability outright.

local text = require("./text")

local prompts = {}

-- Every option is read out. Measured across the whole of src/: the largest
-- query_popup in the game offers four options (game.cpp, the one that asks
-- whether to cancel or ignore an activity), so a prompt is bounded by the game
-- itself and needs no cap of ours. F5 is about lists that can run away -- a
-- recipe browser, a tile-by-tile read -- and a prompt is not one of them.
-- Replacing an option by a count would hide exactly what has to be answered.

--- Strip what is drawn rather than said, plus the one thing only a popup has.
---
--- The bar of a waiting popup is worse than markup: it cycles through | / - \
--- on every redraw, so comparing on the raw text would make every frame look
--- like a new prompt and announce it forever.
--- @param s string|nil
--- @return string
prompts.clean = function(s)
  local out = text.clean(s)
  out = out:gsub("^[|/\\-] ", "")
  return text.clean(out)
end

--- Normalise one firing of the hook into a plain table, with the text already
--- cleaned so that comparisons never see the spinning bar.
--- @param params table
--- @return table
prompts.state = function(params)
  local options = {}
  for i, option in ipairs(params.options or {}) do
    options[i] = { id = option.id, name = option.name }
  end
  return {
    text = prompts.clean(params.text),
    category = params.category,
    options = options,
    cursor = params.cursor,
  }
end

--- Whether two firings describe the same popup still waiting for an answer.
--- The question and the option set identify it; the cursor is what may move
--- within it.
local function same_prompt(state, previous)
  if not previous then return false end
  if state.text ~= previous.text or state.category ~= previous.category then return false end
  if #state.options ~= #previous.options then return false end
  for i = 1, #state.options do
    if state.options[i].id ~= previous.options[i].id then return false end
  end
  return true
end

--- Name first (P2), so the player can cut the speech off as soon as they know
--- enough. The selection is marked rather than reordered, because reordering
--- would make the same option sound different depending on where the cursor is.
local function option_utterance(option, selected)
  if selected then return option.name .. ", selected" end
  return option.name
end

local function selected_utterance(state)
  local option = state.cursor and state.options[state.cursor]
  if not option then return nil end
  return option_utterance(option, true)
end

--- What to say about this firing, given the one before it.
---
--- One utterance, one subject (F1): the question, then each option, never the
--- two interleaved. Nothing at all when nothing changed (P5) -- that silence is
--- what stops a prompt from talking over itself on every keypress.
--- @param state table normalised by prompts.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
prompts.utterances = function(state, previous)
  local out = {}

  if same_prompt(state, previous) then
    if state.cursor ~= previous.cursor then
      local moved = selected_utterance(state)
      if moved then out[#out + 1] = moved end
    end
    return out
  end

  if state.text ~= "" then out[#out + 1] = state.text end

  local count = #state.options
  if count == 0 then return out end

  for i = 1, count do
    out[#out + 1] = option_utterance(state.options[i], i == state.cursor)
  end
  return out
end

return prompts
