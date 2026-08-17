-- What looking at one square sounds like.
--
-- The screen this reads is the one that invites F5: its own panel prints terrain,
-- furniture, fields, traps, the creature, the vehicle, every item and any graffiti
-- for every square the cursor touches. So what is asserted here is as much what is
-- left out as what is said.
--
-- The decisions live in data/access/lib/look.lua and are pure: a table in, a list
-- of strings out.

local look = require("../../../data/access/lib/look")

local function say(state, previous) return table.concat(look.utterances(state, previous), " / ") end

--- The square under the cursor as the hook and the layer hand it over. Defaults
--- describe the firing that opens the screen: the cursor sits on the character, on
--- a square she can see.
local function at(opts)
  return look.state({
    sight = opts.sight or "clear",
    name = opts.name or "grass",
    area = opts.area or "forest",
    creature = opts.creature or "",
    sensed = opts.sensed or {},
    sound = opts.sound or "",
    items = opts.items or 0,
    dx = opts.dx or 0,
    dy = opts.dy or 0,
    dz = opts.dz or 0,
    peeking = opts.peeking == true,
  })
end

check.equal(
  say(at({}), nil),
  "Look around. / Forest. / grass, here.",
  "Opening says the screen, the region it prints above the square, and the square the cursor starts on"
)

check.equal(
  say(at({ peeking = true, name = "wooden door", dx = 1 }), nil),
  "Peeking. / Forest. / wooden door, 1 east.",
  "Peeking is a different act and says so, because what it shows is somewhere she is not standing"
)

check.equal(
  say(at({ name = "wooden door", dx = 4, dy = -4 }), at({})),
  "wooden door, 4 northeast.",
  "Moving the cursor says the square alone, name first, then how far and in which direction"
)

check.equal(
  say(at({ dx = 3 }), at({ dx = 3 })),
  "",
  "A firing that changed nothing says nothing, which is every key the screen ignored"
)

check.equal(
  say(at({ name = "grass", area = "overgrown cabin", dx = 5 }), at({})),
  "Overgrown cabin. / grass, 5 east.",
  "Crossing into another region says the region before the square, as the panel prints it above one"
)

check.equal(
  say(at({ creature = "zombie", name = "grass", dx = 2, dy = 2 }), at({})),
  "zombie on grass, 2 southeast.",
  "A creature comes before the ground it stands on, being the only thing on a square that can act"
)

check.equal(
  say(at({ sight = "dark", name = "wooden door", items = 3, dx = 6 }), at({})),
  "Darkness, 6 east.",
  "A square she cannot make out says so and nothing else: naming what is there would report what she cannot know"
)

check.equal(
  say(at({ sight = "dark", creature = "zombie", sensed = { "You see a human figure" }, dx = 6 }), at({})),
  "Darkness, 6 east. / You see a human figure.",
  "A creature sensed through the dark is said in the game's own words, since the square itself answered only darkness"
)

check.equal(
  say(at({ items = 4, dx = 1, dy = 1 }), at({})),
  "grass, 1 southeast. / 4 items.",
  "What is lying there is a count and never the list: the game has its own key for the list"
)

check.equal(say(at({ items = 1, dx = 1 }), at({})), "grass, 1 east. / 1 item.", "One item is one item")

check.equal(
  say(at({ sight = "hidden", items = 9, dx = 9 }), at({})),
  "Unseen, 9 east.",
  "Items are not counted on a square that cannot be seen, which is what the panel does too"
)

check.equal(
  say(at({ sound = "shouting", dx = 2 }), at({})),
  "grass, 2 east. / Sound: shouting.",
  "Sound is said after the square, being the one channel that reaches through a wall"
)

check.equal(
  say(at({ sight = "hidden", sound = "a scream", dx = 8, dy = 8 }), at({})),
  "Unseen, 8 southeast. / Sound: a scream.",
  "Sound answers at any visibility, which is why it is the only thing said about an unseen square"
)

check.equal(
  say(at({ dz = -1 }), at({})),
  "grass, 1 level down.",
  "A square straight below reads as a level rather than a bearing, since a bearing of zero says nothing"
)

check.equal(
  say(at({ name = "", dx = 3 }), at({})),
  "nothing, 3 east.",
  "A square the game names with nothing at all still answers, because silence would read as a dead key"
)

check.equal(
  say(at({ dx = 3 }), { screen = "menu", title = "Main menu" }),
  "Look around. / Forest. / grass, 3 east.",
  "A remembered state from another screen reads as an arrival, because closing a menu over the map is arriving back"
)
