-- What the list of ways out sounds like.
--
-- The one screen the layer owns instead of speaks, so this is where its reading is
-- pinned: no screen in the game answers which ways lead out of a building and
-- where they are, and a sentence that names only the nearest one cannot be gone
-- back to.
--
-- The decisions live in data/access/lib/exits.lua and are pure: rows in, a list of
-- strings out. The reading model itself is menus.lua and is asserted there.

local exits = require("../../../data/access/lib/exits")

local function say(state, previous) return table.concat(exits.utterances(state, previous), " / ") end

local three = {
  { name = "wooden door", kind = "door", dx = 3, dy = 0 },
  { name = "stairs", kind = "up", dx = -8, dy = -8 },
  { name = "manhole cover", kind = "down", dx = 0, dy = 12 },
}

check.equal(
  say(exits.state(three, 1, "Subway station, ways out"), nil),
  "Subway station, ways out, 3 entries. / wooden door, 3 east, 1 of 3.",
  "Opening names the place and how many ways out it knows of, then the nearest one and which way to walk"
)

check.equal(
  say(exits.state(three, 2, "Subway station, ways out"), exits.state(three, 1, "Subway station, ways out")),
  "stairs up, 8 northwest, 2 of 3.",
  "Stepping down says the row alone, and a staircase says which way it goes -- that is what decides whether to cross a room for it"
)

check.equal(
  say(exits.state(three, 3, "Subway station, ways out"), exits.state(three, 2, "Subway station, ways out")),
  "manhole cover down, 12 south, 3 of 3.",
  "The position in the list says how much is left, so walking off the end is hearable rather than silent"
)

check.equal(
  say(exits.state(three, 1, "Subway station, ways out"), exits.state(three, 1, "Subway station, ways out")),
  "",
  "A firing that changed nothing says nothing, which is every key the screen ignored"
)

check.equal(
  say(exits.state({}, nil, "Forest, ways out"), nil),
  "Forest, ways out, no entries.",
  "A place with no known way out says so, which is the answer that sends her to go and look"
)

check.equal(
  say(exits.state({ { name = "gate", kind = "door", dx = 0, dy = 0 } }, 1, nil), nil),
  "Here, 1 entry. / gate, here, 1 of 1.",
  "A list opened without a name still answers, and a way out underfoot says so rather than giving a bearing of zero"
)

check.equal(
  say(
    exits.state({
      { name = "zombie", kind = "enemy", dx = 0, dy = -3 },
      { name = "wooden door", kind = "door", dx = 5, dy = 0 },
    }, 1, "Nearby"),
    nil
  ),
  "Nearby, 2 entries. / zombie, 3 north, 1 of 2.",
  "The same list reads what is near her too, and a creature carries no extra word: what it is is the whole of it"
)
