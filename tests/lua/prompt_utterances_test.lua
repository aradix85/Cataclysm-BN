-- Drives the prompt-speech decisions from Lua, against the module the mod
-- itself loads. Assertions live on the C++ side; each case writes its result
-- into test_data as one string, with " / " between utterances so that both the
-- content and the count are pinned by one comparison.

local prompts = require("../../data/mods/bn_access/lib/prompts")

local function say(state, previous) return table.concat(prompts.utterances(state, previous), " / ") end

local yesno = function(cursor)
  return prompts.state({
    text = "You may be attacked!  Proceed?",
    category = "YESNO",
    options = { { id = "YES", name = "Yes" }, { id = "NO", name = "No" } },
    cursor = cursor,
  })
end

-- First showing: the question, then one utterance per option, selection marked.
test_data.first_showing = say(yesno(1), nil)

-- The same prompt again with nothing changed. This is what arrives after any
-- key the prompt ignored, and it must be silent.
test_data.unchanged = say(yesno(1), yesno(1))

-- The cursor moved: only the newly selected option, never the question again.
test_data.cursor_moved = say(yesno(2), yesno(1))

-- A waiting popup: colour markup and the spinning bar are drawn, not said.
local waiting = function(bar)
  return prompts.state({
    text = " <color_light_green>" .. bar .. "</color> Please wait\226\128\166",
    category = "POPUP_WAIT",
    options = {},
  })
end

test_data.waiting = say(waiting("|"), nil)

-- The bar cycles on every redraw. If it were compared raw, every frame would
-- look like a new prompt and the game would talk forever.
test_data.waiting_spun = say(waiting("/"), waiting("|"))

-- Too many options to read out. A prompt cannot be escaped, so it stays short.
local many = prompts.state({
  text = "Which direction?",
  category = "MANY",
  options = {
    { id = "N", name = "North" },
    { id = "E", name = "East" },
    { id = "S", name = "South" },
    { id = "W", name = "West" },
    { id = "Q", name = "Cancel" },
  },
  cursor = 3,
})
test_data.many_options = say(many, nil)

-- Nothing to say produces nothing.
test_data.empty = say(prompts.state({ text = "", category = "NONE", options = {} }), nil)
