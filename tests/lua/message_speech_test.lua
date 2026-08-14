-- Drives the message policy from Lua, against the module the mod itself loads.
-- Assertions live on the C++ side; each case writes one string into test_data.

local messages = require("../../data/mods/bn_access/lib/messages")
local text = require("../../data/mods/bn_access/lib/text")

-- Decoration is drawn, not said. A line with no letter and no digit left would
-- otherwise be read out glyph by glyph.
test_data.speaks_prose = tostring(messages.should_speak(MsgType.neutral, "You hit the zombie."))
test_data.speaks_border = tostring(messages.should_speak(MsgType.neutral, text.clean("--\\ // --")))
test_data.speaks_debug = tostring(messages.should_speak(MsgType.debug, "Spawned 3 monsters."))

-- Taking damage must not wait behind a list of things lying on the floor (P7).
test_data.urgency_bad = messages.urgency(MsgType.bad)
test_data.urgency_warning = messages.urgency(MsgType.warning)
test_data.urgency_info = messages.urgency(MsgType.info)
test_data.urgency_good = messages.urgency(MsgType.good)

-- Markup and wrapping are the game drawing, not the game speaking.
test_data.cleaned = text.clean("<color_light_red>You are  bleeding</color>\n  badly. ")
