-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loading this mod is silent on purpose. Nothing is announced until the player
-- asks for it or until the game demands an answer, so the first thing ever
-- heard is either a request or something that has taken the keyboard.
--
-- Registered here: the two scaffolding checks the command dispatch step is
-- judged by, and the blocking prompts, which are not scaffolding. A prompt
-- accepts its own two or four keys and swallows every other one, so a silent
-- prompt makes every command unreachable and is indistinguishable from a dead
-- keyboard for a player who cannot see it.

local speech = require("./lib/speech")
local prompts = require("./lib/prompts")

local mod = game.mod_runtime[game.current_mod]

mod.speech = speech

gapi.register_default_mode_action("bn_access_smoke", "Accessibility: speech check")
gapi.register_default_mode_action("bn_access_context_smoke", "Accessibility: input loop check")

-- One key, one fixed sentence. If this speaks, every stage of the chain works:
-- the key reached the game's own input context, the action string survived
-- registration, the hook fired before the game tried to resolve it, and the
-- speech bridge reached NVDA. If it stays silent, the chain broke somewhere,
-- and nothing else in this mod speaks, so silence has only one meaning.
local function speech_check() speech.say("Speech check. The key reached Lua.") end

-- An input context of the mod's own: it reads keys until dismissed, rather
-- than one command per keypress. This is the loop every browsable list will
-- use later, and it costs no game time.
local function input_loop_check()
  local ctxt = InputContext.new("BN_ACCESS_LOOP")
  ctxt:register_action("NEXT")
  ctxt:register_action("PREV")
  ctxt:register_action("QUIT")

  speech.say("Loop open. J and K report, Q leaves.")
  while true do
    local action = ctxt:handle_input()
    if action == "QUIT" then
      speech.say("Loop closed.")
      return
    elseif action == "NEXT" then
      speech.say("Next.")
    elseif action == "PREV" then
      speech.say("Previous.")
    end
  end
end

-- The prompt that is currently on screen, as prompts.state() normalised it.
-- It is what makes "has anything changed" answerable, since the hook fires
-- again after every keypress the prompt did not accept.
local open_prompt = nil

-- A blocking prompt speaks itself. Nothing is asked of the player first: by the
-- time this fires the game has already taken the keyboard, so an announcement
-- is not an interruption, it is the only way out.
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

-- Registered at a high priority so the layer sees an action before any other
-- mod can alter or veto it. Returning false claims the action: the game then
-- stops before resolving it, so no turn passes. Returning nothing lets the
-- action through untouched, which is what every action that is not ours does.
game.add_hook("on_action", {
  priority = 100,
  fn = function(params)
    -- An action in the default mode context can only arrive while no popup
    -- holds the keyboard, so the prompt that was open is gone. Forgetting it
    -- here is what lets the same question be asked, and answered, twice.
    open_prompt = nil

    if params.action == "bn_access_smoke" then
      speech_check()
      return false
    elseif params.action == "bn_access_context_smoke" then
      input_loop_check()
      return false
    end
  end,
})
