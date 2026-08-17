-- What coming back to the world sounds like.
--
-- Every screen in the game closes without a sound, so a key pressed just after
-- one is a key into an unknown place: the menu may still be there, or the world
-- may have it. Both feel the same, and the wrong guess spends a turn or a step.
--
-- The decision lives in data/access/lib/play.lua and is pure: what was on top in,
-- utterances out.

local play = require("../../../data/access/lib/play")

local function say(what) return table.concat(play.utterances(what), " / ") end

check.equal(
  say({ screen = true }),
  "World.",
  "A screen that has just closed is answered by the world saying it has the keyboard"
)

check.equal(say({ screen = false }), "", "Ordinary play says nothing: this fires before every single action she takes")

check.equal(say({}), "", "Nothing on top reads as nothing on top, so a caller that cannot tell stays silent")
