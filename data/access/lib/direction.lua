-- The question the game asks when a verb needs a square: "Close where?", "Smash
-- where?", "Jump across where?".
--
-- Pure: a table in, a list of utterances out. No game state, no gapi.
--
-- This prompt takes the keyboard as completely as a blocking popup does, and until
-- it is answered nothing else in the game reads a key. Unspoken it is the worst
-- shape there is without sight: the next arrow key is swallowed by a question
-- nobody heard, and a keyboard that has stopped working and a keyboard waiting for
-- an answer feel exactly alike.

local text = require("./text")

local direction = {}

--- The firing as a table worth comparing: the question, and whether up and down
--- answer it as well as the eight compass keys.
--- @param params table { text, vertical, action }
--- @return table
direction.state = function(params)
  return {
    text = text.clean(params.text or ""),
    vertical = params.vertical == true,
  }
end

--- What to say about this firing, given the one before it.
---
--- The question first and alone (P2, F1), so speech can be cut off the moment it is
--- clear what is being asked.
---
--- How to answer is said only on arrival. It is not a keybinding hint of the kind
--- F4 forbids -- that is a header read before the content -- but the one thing this
--- screen needs and never says: which family of keys it is waiting for. Said after
--- the question rather than before it, and not repeated, because a verb used twenty
--- times a session should not explain itself twenty times.
---
--- A second firing means the previous key did not answer the prompt, since any key
--- that does ends the loop. So the question comes again: it is still standing, and
--- silence there would read as the answer having been taken.
--- @param state table normalised by direction.state
--- @param previous table|nil the state of the firing before this one
--- @return string[]
direction.utterances = function(state, previous)
  if state.text == "" then return {} end

  local out = { state.text }

  if not previous or previous.text ~= state.text then
    if state.vertical then
      out[#out + 1] = "Direction key, or up or down."
    else
      out[#out + 1] = "Direction key."
    end
  end

  return out
end

return direction
