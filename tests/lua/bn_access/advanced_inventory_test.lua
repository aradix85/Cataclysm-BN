-- What the advanced inventory sounds like.
--
-- The screen items are moved on, and the only screen in the game whose whole
-- subject is a pair of places rather than a list. Which pane holds the cursor,
-- what each pane is aimed at, and which square a row's item is lying on are all
-- drawn as position and colour, so every one of them is silence until it is
-- said here.
--
-- The decisions live in data/access/lib/advanced_inventory.lua and are pure:
-- tables in, list of strings out. That is what makes them assertable here, with
-- no game running and no NVDA present.

local advanced = require("../../../data/access/lib/advanced_inventory")

--- One firing's utterances as a single string, so that both the content and the
--- count of them are pinned by one comparison.
local function say(state, previous) return table.concat(advanced.utterances(state, previous), " / ") end

--- The screen as the hook hands it over: the two places, how many rows the
--- active pane holds, and the row the cursor is on.
local function screen(opts)
  return advanced.state({
    source = { area = opts.from or "Inventory", vehicle = opts.from_vehicle or "" },
    destination = { area = opts.to or "South", vehicle = opts.to_vehicle or "" },
    count = opts.count or 4,
    cursor = opts.cursor,
    entry = opts.name and {
      text = opts.name,
      category = opts.group or "",
      square = opts.square or "",
    } or nil,
  })
end

check.equal(
  say(screen({ cursor = 1, name = "pocket knife" }), nil),
  "From Inventory to South, 4 entries. / pocket knife, 1 of 4.",
  "Opening says both places at once and how much is in the one being read, then the row the cursor sits on"
)

check.equal(
  say(screen({ cursor = 2, name = "rock" }), screen({ cursor = 1, name = "pocket knife" })),
  "rock, 2 of 4.",
  "Moving the cursor says the row alone -- name first, then where it sits"
)

check.equal(
  say(screen({ cursor = 1, name = "pocket knife" }), screen({ cursor = 1, name = "pocket knife" })),
  "",
  "The same screen redrawn after a key it ignored says nothing, so it cannot talk over itself"
)

check.equal(
  say(screen({ cursor = 1, name = "rock", count = 6, to = "North" }), screen({ cursor = 1, name = "pocket knife" })),
  "From Inventory to North, 6 entries. / rock, 1 of 6.",
  "Aiming the receiving pane somewhere else is arriving somewhere, so both places and the size are said again"
)

check.equal(
  say(
    screen({ cursor = 1, name = "rock", count = 6, from = "South", to = "Inventory" }),
    screen({ cursor = 1, name = "pocket knife" })
  ),
  "From South to Inventory, 6 entries. / rock, 1 of 6.",
  "Swapping which pane holds the cursor reverses the pair, which is the one thing that screen is for"
)

check.equal(
  say(screen({ cursor = 1, name = "rock", from = "South", from_vehicle = "shopping cart" }), nil),
  "From shopping cart at South to South, 4 entries. / rock, 1 of 4.",
  "A pane showing a vehicle's cargo names the vehicle and keeps the direction, which is what says where to walk"
)

check.equal(
  say(
    screen({ cursor = 2, name = "rock", group = "TOOLS" }),
    screen({ cursor = 1, name = "pocket knife", group = "WEAPONS" })
  ),
  "Tools. / rock, 2 of 4.",
  "Sorted by category, crossing into another heading says it before the row and never shouts it"
)

check.equal(
  say(screen({ cursor = 3, name = "sock", group = "TOOLS" }), screen({ cursor = 2, name = "rock", group = "TOOLS" })),
  "sock, 3 of 4.",
  "The heading is said when it changes and never otherwise, so it costs one sentence per group and not per row"
)

check.equal(
  say(
    screen({ cursor = 2, name = "rock", square = "South West" }),
    screen({ cursor = 1, name = "pocket knife", square = "South" })
  ),
  "South West. / rock, 2 of 4.",
  "Reading every square at once says which one a row is lying on, which the screen answers with two drawn letters"
)

check.equal(
  say(
    screen({ cursor = 2, name = "rock", group = "TOOLS", square = "South West" }),
    screen({ cursor = 1, name = "pocket knife", group = "WEAPONS", square = "South" })
  ),
  "Tools, South West. / rock, 2 of 4.",
  "Heading and square answer the same question and are one sentence, not two"
)

check.equal(
  say(screen({ count = 0 }), screen({ cursor = 1, name = "pocket knife" })),
  "No entries.",
  "A filter that leaves nothing says so, since a pane with no rows answers every arrow key with silence"
)
