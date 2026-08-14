-- What to do with a message the game just logged.
--
-- Pure: values in, values out. No speech and no gapi, so the policy is
-- assertable without a game. Which channel an utterance leaves through is
-- main.lua's business; whether it should leave at all, and how urgently, is
-- decided here.

local text = require("./text")

local messages = {}

--- Whether this message is for the player at all.
---
--- Debug messages are for a developer with the console open and would bury the
--- player in noise. Anything with no letter or digit left after cleaning is
--- decoration, not prose.
--- @param msg_type any a MsgType value
--- @param body string the cleaned message text
--- @return boolean
messages.should_speak = function(msg_type, body)
  if msg_type == MsgType.debug then return false end
  return text.is_speakable(body)
end

--- How far ahead of whatever is already queued this message should go.
---
--- Returned as a name rather than a priority value so that this module stays
--- free of gapi. Speech queues at NVDA and nothing in the game throttles it
--- (P8), so a message that matters has to be able to pass the ones that do not:
--- taking damage must not wait behind a list of things lying on the floor (P7).
---
--- Only "next" is used, never "now". "now" discards what is already speaking,
--- and in a fight every hit is bad news, so each would silence the one before
--- it and only the last would ever be heard.
--- @param msg_type any a MsgType value
--- @return string "next" or "normal"
messages.urgency = function(msg_type)
  if msg_type == MsgType.bad or msg_type == MsgType.warning then return "next" end
  return "normal"
end

return messages
