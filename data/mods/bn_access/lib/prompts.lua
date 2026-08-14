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

local prompts = {}

-- Beyond this many options, the list is replaced by a count. A prompt is the
-- one place the player cannot escape, so it must stay short (F5).
local MAX_LISTED_OPTIONS = 4

--- Strip what is drawn rather than said.
---
--- Colour tags are markup. The bar of a waiting popup is worse than noise: it
--- cycles through | / - \ on every redraw, so comparing on the raw text would
--- make every frame look like a new prompt and announce it forever.
--- @param text string|nil
--- @return string
prompts.clean = function(text)
  if not text or text == "" then return "" end
  local out = text:gsub("</?color[^>]*>", "")
  out = out:gsub("^%s*[|/\\-]%s+", "")
  out = out:gsub("%s+", " ")
  out = out:gsub("^ ", ""):gsub(" $", "")
  return out
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

  if count > MAX_LISTED_OPTIONS then
    out[#out + 1] = string.format("%d options", count)
    local selected = selected_utterance(state)
    if selected then out[#out + 1] = selected end
    return out
  end

  for i = 1, count do
    out[#out + 1] = option_utterance(state.options[i], i == state.cursor)
  end
  return out
end

return prompts
