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

local speech = {}

--- Speak and braille the same text.
--- @param text string
--- @param prio integer|nil gapi.speech_priority_normal (default), _next or _now
speech.say = function( text, prio )
  gapi.speak( text, prio )
end

--- Speak one text and braille a different one. Only for cases where the two
--- genuinely have to differ; whether any such case exists is still open.
speech.say_split = function( spoken, brailled, prio )
  gapi.speak_split( spoken, brailled, prio )
end

speech.speech_only = function( text, prio )
  gapi.speech_only( text, prio )
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
