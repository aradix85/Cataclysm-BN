-- What the game's own error report says.
--
-- Pure: the error text in, utterances out.
--
-- A debug report is how the game reports a fault, and it is also how a fault in
-- this layer reaches the player. It draws over the screen, says nothing, and
-- swallows every key until space, i or c arrives. Without sight that is
-- indistinguishable from a dead keyboard, and it can be raised from anywhere in
-- the game, so it has to speak before anything built on top of it can be
-- trusted to.
--
-- That screen reads raw input, so the layer can neither add a key of its own
-- here nor offer a second level of detail on request (P1). One firing carries
-- both what happened and the way out.

local text = require("./text")

local errors = {}

-- Both keys are named, because a report that keeps coming back is what ends a
-- session: space answers this one, i silences that particular error until the
-- game is restarted.
local WAY_OUT = "Space to continue, or I to ignore this error."

-- A fault in Lua arrives with the file, the line and often a traceback behind
-- it. The first line is what happened; the rest is where it happened, written
-- for a developer reading a screen, and the debug log keeps all of it.
local LIMIT = 200

--- The part of a report worth hearing, and whether anything was left out.
--- @param s string|nil
--- @return string, boolean
errors.summary = function(s)
  local first = text.clean((s or ""):match("^[^\n]*"))
  if #first <= LIMIT then return first, false end
  -- Cut at a word boundary, so the last thing heard is a word and not half of
  -- one, which a synthesiser pronounces as something else entirely.
  return (first:sub(1, LIMIT):gsub("%s%S*$", "")), true
end

--- What to say about a report, given the one before it.
---
--- Name first (P2): the word "Error" says which screen is up, before the text
--- that explains it, because knowing the keyboard is held matters more than
--- knowing why. The way out is a second utterance rather than a longer first
--- one, so it is the same short sentence every time and can be recognised
--- without being listened to.
---
--- The game shows the same report a second time once it repeats too often, and
--- a fault in a hook repeats every turn. Saying the whole text again is F5, so
--- an identical report is answered by naming it as one.
--- @param current string|nil the error as the game reported it
--- @param previous string|nil the error reported before it, if any
--- @return string[]
errors.utterances = function(current, previous)
  if previous and current == previous then return { "Same error again.", WAY_OUT } end

  local summary, cut = errors.summary(current)
  if not text.is_speakable(summary) then return { "Error.", WAY_OUT } end

  local line = "Error. " .. summary
  if not line:find("[.!?]$") then line = line .. "." end
  if cut then line = line .. " The rest is in the log." end

  return { line, WAY_OUT }
end

return errors
