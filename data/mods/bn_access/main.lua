-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loading this mod is silent on purpose. It speaks when the game says something
-- to the player and when the game demands an answer. Nothing else, and nothing
-- is asked of the player first.
--
-- Blocking prompts speak because a prompt accepts its own two or four keys and
-- swallows every other one, so a silent prompt makes every command unreachable.
-- The message log speaks because that is the game telling the player what just
-- happened to them.
--
-- The mod owns no keys. Everything here is the game talking; nothing has to be
-- requested, so there is nothing to press.

local speech = require("./lib/speech")
local prompts = require("./lib/prompts")
local messages = require("./lib/messages")
local text = require("./lib/text")

local mod = game.mod_runtime[game.current_mod]

mod.speech = speech

-- The message log speaks. Registered at a high priority so the layer sees the
-- message before another mod can alter or veto it.
game.add_hook("on_add_msg", {
  priority = 100,
  fn = function(params)
    local body = text.clean(params.text)
    if not messages.should_speak(params.type, body) then return end

    if messages.urgency(params.type) == "next" then
      speech.say(body, gapi.speech_priority_next())
    else
      speech.say(body)
    end
  end,
})

-- The prompt that is currently on screen, as prompts.state() normalised it.
-- It is what makes "has anything changed" answerable, since the hook fires
-- again after every keypress the prompt did not accept.
local open_prompt = nil

-- A blocking prompt speaks itself. By the time this fires the game has already
-- taken the keyboard, so an announcement is not an interruption, it is the only
-- way out.
game.add_hook("on_query_popup", {
  priority = 100,
  fn = function(params)
    local state = prompts.state(params)
    for _, line in ipairs(prompts.utterances(state, open_prompt)) do
      speech.say(line)
    end
    open_prompt = state
  end,
})

-- An action in the default mode context can only arrive while no popup holds
-- the keyboard, so the prompt that was open is gone. Forgetting it here is what
-- lets the same question be asked, and answered, twice.
game.add_hook("on_action", {
  priority = 100,
  fn = function() open_prompt = nil end,
})
