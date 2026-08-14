-- Drives the message policy from Lua, against the module the mod itself loads.
-- Assertions live on the C++ side; each case writes one string into test_data.

local messages = require("../../data/mods/bn_access/lib/messages")
local text = require("../../data/mods/bn_access/lib/text")

-- Decoration is drawn, not said. A line with no letter and no digit left would
-- otherwise be read out glyph by glyph.
test_data.speaks_prose = tostring(messages.should_speak(MsgType.neutral, "You hit the zombie."))
test_data.speaks_border = tostring(messages.should_speak(MsgType.neutral, text.clean("--\\ // --")))
test_data.speaks_debug = tostring(messages.should_speak(MsgType.debug, "Spawned 3 monsters."))

-- Markup and wrapping are the game drawing, not the game speaking.
test_data.cleaned = text.clean("<color_light_red>You are  bleeding</color>\n  badly. ")
