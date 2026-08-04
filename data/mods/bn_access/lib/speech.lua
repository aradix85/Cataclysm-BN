-- Speech and braille for bn_access.
--
-- A thin pass-through to the C++ bridge, kept as a module so that call sites
-- never touch gapi directly: swapping or wrapping the transport later is then
-- one file, not a search across the mod.
--
-- say() reaches speech and braille together and is the call to use. The
-- single-channel calls are named "_only" so that using one is a choice rather
-- than an oversight -- losing braille by forgetting it is the failure this
-- naming exists to prevent.
--
-- Priorities come from gapi.speech_priority_normal(), _next() and _now().
-- They are functions, following the convention of the rest of gapi.
--
-- Every wrapper omits the priority argument entirely rather than passing nil:
-- the bindings are overloaded on arity, and a nil second argument matches
-- neither overload.

local speech = {}

--- Speak and braille the same text.
--- @param text string
--- @param prio integer|nil
speech.say = function( text, prio )
  if prio then
    gapi.speak( text, prio )
  else
    gapi.speak( text )
  end
end

--- Speak one text and braille a different one. Only for cases where the two
--- genuinely have to differ; whether any such case exists is still open.
speech.say_split = function( spoken, brailled, prio )
  if prio then
    gapi.speak_split( spoken, brailled, prio )
  else
    gapi.speak_split( spoken, brailled )
  end
end

speech.speech_only = function( text, prio )
  if prio then
    gapi.speech_only( text, prio )
  else
    gapi.speech_only( text )
  end
end

speech.braille_only = function( text )
  gapi.braille_only( text )
end

--- Drop everything queued and stop talking now.
speech.silence = function()
  gapi.cancel_speech()
end

--- True when NVDA is reachable. Costs a round trip; do not call per utterance.
speech.available = function()
  return gapi.speech_available()
end

return speech
