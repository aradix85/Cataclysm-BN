-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loading this mod is silent on purpose. Nothing is announced until the player
-- asks for it, so the first thing ever heard is something that was requested.
--
-- What is registered here are the two checks the command dispatch step is
-- judged by: one key that speaks a fixed string during ordinary play, and one
-- that opens an input context of the mod's own and reads keys until dismissed.
-- Both are scaffolding for the real commands and are meant to be replaced.

local speech = require("lib.speech")

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

-- Registered at a high priority so the layer sees an action before any other
-- mod can alter or veto it. Returning false claims the action: the game then
-- stops before resolving it, so no turn passes. Returning nothing lets the
-- action through untouched, which is what every action that is not ours does.
game.add_hook("on_action", {
  priority = 100,
  fn = function(params)
    if params.action == "bn_access_smoke" then
      speech_check()
      return false
    elseif params.action == "bn_access_context_smoke" then
      input_loop_check()
      return false
    end
  end,
})
