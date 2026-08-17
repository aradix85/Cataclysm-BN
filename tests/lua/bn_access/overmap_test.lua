-- What the overmap sounds like.
--
-- The screen that turns the world into named places, and the one whose whole
-- subject is a place rather than a thing. Everything it draws about a tile is
-- position and colour: where the cursor is, whether that tile was ever seen,
-- whether it has been explored, and whether a route to it has been laid out. All
-- of it is silence until it is said here.
--
-- The decisions live in data/access/lib/overmap.lua and are pure: a table in, a
-- list of strings out. That is what makes them assertable with no world loaded.

local overmap = require("../../../data/access/lib/overmap")

--- One firing's utterances as a single string, so that both the content and the
--- count of them are pinned by one comparison.
local function say(state, previous) return table.concat(overmap.utterances(state, previous), " / ") end

--- The cursor as the hook hands it over. Defaults describe the firing that opens
--- the screen: the cursor sits on the character, on a tile she has seen.
local function at(opts)
  return overmap.state({
    place = opts.place or "forest",
    note = opts.note or "",
    seen = opts.seen ~= false,
    explored = opts.explored == true,
    dx = opts.dx or 0,
    dy = opts.dy or 0,
    dz = opts.dz or 0,
    route = opts.route or 0,
    action = opts.action or "",
  })
end

check.equal(
  say(at({}), nil),
  "Overmap. / forest, here.",
  "Opening says the screen once and then the tile the cursor starts on, which is the character's own"
)

check.equal(
  say(at({ place = "house in central Springfield", dx = 4, dy = -4 }), nil),
  "Overmap. / house in central Springfield, 4 northeast.",
  "The place is said in the game's own words, name first, then how far away it is and in which direction"
)

check.equal(
  say(at({ dx = 3 }), at({})),
  "forest, 3 east.",
  "Moving the cursor says the tile alone: the screen names itself once and never again"
)

check.equal(
  say(at({ dx = 3 }), at({ dx = 3 })),
  "",
  "A firing that changed nothing says nothing, which is every key the screen ignored"
)

check.equal(
  say(at({ seen = false, place = "", dx = 0, dy = -7 }), at({})),
  "Unexplored, 7 north.",
  "A tile never seen says so instead of naming itself, which is how the edge of the known map is found"
)

check.equal(
  say(at({ explored = true, dx = -2 }), at({})),
  "forest, 2 west, explored.",
  "Explored is drawn as a colour and said nowhere, so it is said here, after where the tile is"
)

check.equal(
  say(at({ dz = -1 }), at({})),
  "forest, 1 level down.",
  "A tile straight below reads as a level rather than a bearing, since a bearing of zero would say nothing"
)

check.equal(
  say(at({ dx = 6, dy = 6, dz = 2 }), at({})),
  "forest, 6 southeast, 2 levels up.",
  "Distance and level are both said when both differ, in that order"
)

check.equal(
  say(at({ dx = 5, note = "shelter" }), at({})),
  "forest, 5 east. / Note: shelter.",
  "Her own note is a sentence of its own, after the tile it belongs to"
)

check.equal(
  say(at({ dx = 5, note = "shelter" }), at({ dx = 5 })),
  "Note: shelter.",
  "Writing a note on the tile already being read says the note and not the tile again"
)

check.equal(
  say(at({ dx = 5, note = "shelter" }), at({ dx = 5, note = "shelter" })),
  "",
  "A note that has already been said is not said again while the cursor stays where it is"
)

check.equal(
  say(at({ dx = 12, route = 12, action = "CHOOSE_DESTINATION" }), at({ dx = 12 })),
  "Route: 12 tiles.",
  "The travel key only draws a path on its first press, so the length of that path is what answers it"
)

check.equal(
  say(at({ dx = 1, route = 1, action = "CHOOSE_DESTINATION" }), at({ dx = 1 })),
  "Route: 1 tile.",
  "One tile is one tile"
)

check.equal(
  say(
    at({ dx = 12, route = 12, action = "CHOOSE_DESTINATION" }),
    at({ dx = 12, route = 12, action = "CHOOSE_DESTINATION" })
  ),
  "",
  "A route already said is not said again, which is what pressing the travel key a second time does"
)

check.equal(
  say(at({ dx = 9, dy = 9, action = "CHOOSE_DESTINATION" }), at({ dx = 9, dy = 9 })),
  "No route.",
  "The travel key with no route to give answers in words, because the screen answers it with nothing at all"
)

check.equal(
  say(
    at({ seen = false, place = "", dx = 0, dy = -20, action = "CHOOSE_DESTINATION" }),
    at({ seen = false, place = "", dx = 0, dy = -20 })
  ),
  "Not on the map yet.",
  "A refusal on a tile the map does not hold yet says so, since going to look is what answers it and no other way round will"
)

check.equal(
  say(at({ action = "CHOOSE_DESTINATION" }), at({})),
  "You are here.",
  "The travel key pressed without moving the cursor says where the cursor is, rather than blaming the route"
)

check.equal(
  say(at({ dx = 4, action = "TOGGLE_HORDES" }), at({ dx = 4 })),
  "",
  "A key that is not the travel key and changed nothing about the tile stays silent"
)

check.equal(
  say(at({ dx = 8, route = 12 }), at({ dx = 4 })),
  "forest, 8 east. / Route: 12 tiles.",
  "A route is said again when the cursor arrives back on the tile it leads to, since it is news there"
)

check.equal(
  say(at({ dx = 3 }), { screen = "menu", title = "Main menu" }),
  "Overmap. / forest, 3 east.",
  "A remembered state belonging to another screen reads as an arrival, because closing a menu over the map is arriving back at it"
)
