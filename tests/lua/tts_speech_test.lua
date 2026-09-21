-- Drives the speech bridge from Lua, so the test proves the whole chain:
-- Lua call -> gapi binding -> tts sink. The assertions live on the C++ side,
-- which installs a recording sink before running this and reads it afterwards.

gapi.speak("three zombies")
gapi.speech_only("spoken only")
gapi.braille_only("brailled only")
gapi.speak_split("the pharmacy on Bell Street", "Bell St pharmacy, C4", gapi.speech_priority_now())
gapi.cancel_speech()
