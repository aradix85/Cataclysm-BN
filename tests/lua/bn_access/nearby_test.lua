-- What the list of nearby items sounds like.
--
-- The game's own answer to "is there anything around here worth walking to". Every
-- row is a name the layer already knows how to say and a direction it says nowhere
-- else on that screen: the screen draws a trail across the map to the selected row
-- and prints no direction at all, so without this the list is a catalogue of names
-- with no way to reach any of them.
--
-- The decisions live in data/access/lib/nearby.lua and are pure: tables in, a list
-- of strings out. The reading model itself is menus.lua and is asserted there; what
-- is asserted here is the place, and that it lands where the model already speaks.

local nearby = require("../../../data/access/lib/nearby")

local function say(state, previous) return table.concat(nearby.utterances(state, previous), " / ") end

--- The screen as the hook hands it over.
local function list(opts)
  return nearby.state({
    count = opts.count or 3,
    cursor = opts.cursor,
    entry = opts.name and {
      text = opts.name,
      category = opts.group or "",
      count = opts.stack or 1,
      dx = opts.dx or 0,
      dy = opts.dy or 0,
      dz = opts.dz or 0,
    } or nil,
  })
end

check.equal(
  say(list({ cursor = 1, name = "rock", dy = -4 }), nil),
  "Items nearby, 3 entries. / rock, 4 north, 1 of 3.",
  "Opening names the list and its size, then the row: what it is, which way to walk, and where in the list"
)

check.equal(
  say(list({ cursor = 2, name = "2 bandages", dx = 3, dy = 3 }), list({ cursor = 1, name = "rock", dy = -4 })),
  "2 bandages, 3 southeast, 2 of 3.",
  "Moving down says the row alone, with its own direction: the screen draws a trail there and says nothing"
)

check.equal(
  say(list({ cursor = 1, name = "rock", dy = -4 }), list({ cursor = 1, name = "rock", dy = -4 })),
  "",
  "A firing that changed nothing says nothing, which is every key the screen ignored"
)

check.equal(
  say(list({ cursor = 1, name = "rock" }), nil),
  "Items nearby, 3 entries. / rock, here, 1 of 3.",
  "An item on her own square says so, since it is already at her feet and has no direction"
)

check.equal(
  say(list({ cursor = 1, name = "hammer", dx = 2, dz = -1 }), nil),
  "Items nearby, 3 entries. / hammer, 2 east, 1 level down, 1 of 3.",
  "An item a floor below carries the level too, which no bearing can say"
)

check.equal(
  say(list({ count = 0 }), nil),
  "Items nearby, no entries.",
  "An empty list says it is empty rather than leaving a keypress unanswered"
)

check.equal(
  say(
    list({ count = 1, cursor = 1, name = "rock", dx = 9, dy = 9 }),
    list({ count = 12, cursor = 1, name = "rock", dx = 9, dy = 9 })
  ),
  "1 entry. / rock, 9 southeast, 1 of 1.",
  "Typing a filter answers with how much is left and then the row, whose position in the list has changed with it"
)
