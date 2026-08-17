-- What the game's own detail screen sounds like.
--
-- The screen the describe key opens over a square: a full description of the
-- creature, the furniture or the terrain, with what can be harvested from it and
-- what it deconstructs into. The screen prints it all at once, scrolls nowhere, and
-- accepts only its three keys and the way out -- so nothing can be paged back to
-- and nothing can be asked for twice.
--
-- What is asserted here is therefore the shape of a long answer rather than a short
-- one: that the prose arrives in pieces, that the game's own markup is not read out
-- as words, and that pressing a key the screen ignored says nothing at all.
--
-- The decisions live in data/access/lib/describe.lua and are pure.

local describe = require("../../../data/access/lib/describe")

local function say(state, previous) return table.concat(describe.utterances(state, previous), " / ") end

local function screen(opts)
  return describe.state({
    target = opts.target or "terrain",
    text = opts.text,
    signage = opts.signage or "",
  })
end

check.equal(
  say(screen({ text = "--\nThis is a wooden door.\n--\nA sturdy door.\n--" }), nil),
  "Terrain. / This is a wooden door. / A sturdy door.",
  "The prose arrives one line per utterance, so speech can be cut off at any of them, and the rules between them are dropped"
)

check.equal(
  say(screen({ target = "creature", text = "This is a zombie.\nIt is <good>badly hurt</good>." }), nil),
  "Creature. / This is a zombie. / It is badly hurt.",
  "The game's markup for good and bad news is not read out as words, since a synthesiser would say them"
)

check.equal(
  say(screen({ target = "creature", text = "" }), nil),
  "No creature here.",
  "Nothing of that kind on the square is an answer, not a failure: she pressed the creature key on an empty square"
)

check.equal(
  say(screen({ text = "A sturdy door." }), screen({ text = "A sturdy door." })),
  "",
  "A key the screen ignored says nothing, which is every key but its own three"
)

check.equal(
  say(
    screen({ target = "furniture", text = "This is a stove." }),
    screen({ target = "terrain", text = "A wooden floor." })
  ),
  "Furniture. / This is a stove.",
  "Pressing one of the three keys says which of them is answering, since the description alone does not say"
)

check.equal(
  say(screen({ text = "A signpost.", signage = "Welcome to Springfield" }), nil),
  "Terrain. / A signpost. / Sign: Welcome to Springfield.",
  "What is written on the square comes last, being the one thing here that is not a description"
)

check.equal(
  say(screen({ target = "furniture", text = "", signage = "Keep out" }), nil),
  "No furniture here. / Sign: Keep out.",
  "A sign is said even when there is nothing of the asked-for kind, because the screen appends it either way"
)
