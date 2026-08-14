-- What to do with a message the game just logged.
--
-- Pure: values in, values out. No speech and no gapi, so the policy is
-- assertable without a game. Which channel an utterance leaves through is
-- main.lua's business; whether it should leave at all is decided here.

local text = require("./text")

local messages = {}

--- Whether this message is for the player at all.
---
--- Debug messages are for a developer with the console open and would bury the
--- player in noise. Anything with no letter or digit left after cleaning is
--- decoration, not prose.
---
--- Nothing here decides how urgently a message is spoken. Everything goes out
--- at the ordinary priority and is heard in the order it happened, which is
--- what a run of short sentences wants. Priority exists to interrupt something
--- long, and nothing long exists yet.
--- @param msg_type any a MsgType value
--- @param body string the cleaned message text
--- @return boolean
messages.should_speak = function(msg_type, body)
  if msg_type == MsgType.debug then return false end
  return text.is_speakable(body)
end

return messages
