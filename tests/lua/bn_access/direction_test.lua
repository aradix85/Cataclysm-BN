-- The question a verb asks when it needs a square: what it sounds like, and what it
-- sounds like when the same question is still standing.
--
-- Open, close, grab, smash, peek and most of what an item is used on arrive at one
-- prompt, and it is neither a menu nor a blocking popup: it draws its own window and
-- reads its own keys. Unheard, it is a keyboard that has stopped working.

local direction = require("../../../data/access/lib/direction")

local function say(lines) return table.concat(lines, " / ") end

check.equal(
  say(direction.utterances(direction.state({ text = "Close where?", vertical = false }))),
  "Close where? / Direction key.",
  "Arriving says the question in the game's own words and then which keys answer it"
)

check.equal(
  say(direction.utterances(direction.state({ text = "Jump across where?", vertical = true }))),
  "Jump across where? / Direction key, or up or down.",
  "Where the prompt takes up and down as well, that is said with it, since it is a different set of keys"
)

local standing = direction.state({ text = "Close where?", vertical = false })

check.equal(
  say(direction.utterances(direction.state({ text = "Close where?", vertical = false }), standing)),
  "Close where?",
  "A key that did not answer says the question again and not the keys, so a verb used twenty times does not explain itself twenty times"
)

check.equal(
  say(direction.utterances(direction.state({ text = "Smash where?", vertical = false }), standing)),
  "Smash where? / Direction key.",
  "Another question is another prompt, so it is owed the whole answer even while one was just met"
)

check.equal(
  say(direction.utterances(direction.state({ text = "" }))),
  "",
  "A prompt with no question of its own says nothing, rather than a bare instruction about keys"
)
