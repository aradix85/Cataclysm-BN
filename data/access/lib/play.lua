-- Coming back to the world.
--
-- Pure: what was on top in, utterances out.
--
-- Every screen in the game closes without a sound. The screen stops answering
-- keys, the world starts answering them again, and nothing at all marks the
-- change -- which costs nothing while it can be seen, and costs the next keypress
-- otherwise: a key pressed into a menu that has already closed does something
-- else entirely, and a key pressed into a menu that has not closed yet does
-- nothing. Neither is distinguishable from the other by ear.
--
-- So the world says once that it has the keyboard. One word, and the same word
-- whatever closed: naming the screen that went away would tell her what she just
-- pressed the key to do, while what she cannot know is where the next key lands.
--
-- A blocking prompt is deliberately not answered here. A prompt is answered by a
-- key it named itself, and what follows is the result of the thing she agreed to,
-- which arrives as a message and speaks. Saying a word after each of those would
-- put a second sentence on every yes and no in the game.

local play = {}

--- @param what table { screen = boolean|nil } whether a screen was on top
--- @return string[]
play.utterances = function(what)
  if not what.screen then return {} end
  return { "World." }
end

return play
