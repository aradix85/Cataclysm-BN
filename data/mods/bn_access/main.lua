-- bn_access: self-voicing accessibility layer for Cataclysm: Bright Nights.
--
-- Loading this mod is silent on purpose. It speaks when the game says something
-- to the player, when the game demands an answer, and when the player asks to
-- hear something again. Nothing else.
--
-- Two things speak by themselves. Blocking prompts, because a prompt accepts
-- its own two or four keys and swallows every other one, so a silent prompt
-- makes every command unreachable. And the message log, because that is the
-- game telling the player what just happened to them.
--
-- Everything spoken is kept in a scrollback and can be read again (P9). In NVDA
-- sleep mode nothing else can recover it.

local speech = require("./lib/speech")
local prompts = require("./lib/prompts")
local messages = require("./lib/messages")
local scrollback = require("./lib/scrollback")
local text = require("./lib/text")

local mod = game.mod_runtime[game.current_mod]

mod.speech = speech

local heard = scrollback.new()

gapi.register_default_mode_action("bn_access_older", "Accessibility: previous message")
gapi.register_default_mode_action("bn_access_newer", "Accessibility: next message")
gapi.register_default_mode_action("bn_access_repeat", "Accessibility: repeat message")

-- The message log speaks. Registered at a high priority so the layer sees the
-- message before another mod can alter or veto it.
game.add_hook("on_add_msg", {
  priority = 100,
  fn = function(params)
    local body = text.clean(params.text)
    if not messages.should_speak(params.type, body) then return end

    scrollback.add(heard, body)

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

-- A blocking prompt speaks itself. Nothing is asked of the player first: by the
-- time this fires the game has already taken the keyboard, so an announcement
-- is not an interruption, it is the only way out.
game.add_hook("on_query_popup", {
  priority = 100,
  fn = function(params)
    local state = prompts.state(params)
    for _, line in ipairs(prompts.utterances(state, open_prompt)) do
      scrollback.add(heard, line)
      speech.say(line)
    end
    open_prompt = state
  end,
})

-- Paging the scrollback. One entry per keypress, speech and braille together,
-- and a word at each end rather than a silent wrap -- a wrap with no signal
-- reads as a stuck key.
local function step(move, end_word)
  local line = move(heard)
  if line then
    speech.say(line)
  elseif scrollback.is_empty(heard) then
    speech.say("Nothing yet.")
  else
    speech.say(end_word)
  end
end

-- Answering "nothing yet" rather than staying silent is deliberate. P5 is about
-- not volunteering emptiness; this was asked for, and a silent answer cannot be
-- told apart from a key that never arrived.
local function repeat_current()
  local line = scrollback.current(heard)
  if line then
    speech.say(line)
  else
    speech.say("Nothing yet.")
  end
end

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

    if params.action == "bn_access_older" then
      step(scrollback.older, "Oldest.")
      return false
    elseif params.action == "bn_access_newer" then
      step(scrollback.newer, "Newest.")
      return false
    elseif params.action == "bn_access_repeat" then
      repeat_current()
      return false
    end
  end,
})
