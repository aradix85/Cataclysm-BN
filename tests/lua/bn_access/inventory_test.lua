-- What the inventory sounds like.
--
-- One hook carries every screen built on inventory_selector -- the plain
-- inventory, wield, wear, eat, drop, pick up, use -- so this wording is not one
-- screen's. It is the last system in the game that said nothing at all, so
-- every line here is the difference between a screen that can be used and a
-- screen that answers every keypress with silence.
--
-- The decisions live in data/access/lib/inventory.lua and are pure: tables in,
-- list of strings out. That is what makes them assertable here, with no game
-- running and no NVDA present.

local inventory = require("../../../data/access/lib/inventory")

--- One firing's utterances as a single string, so that both the content and the
--- count of them are pinned by one comparison.
local function say(state, previous) return table.concat(inventory.utterances(state, previous), " / ") end

--- The screen as the hook hands it over: a title, how many rows the filter
--- leaves, and the row the cursor is on.
local function screen(opts)
  return inventory.state({
    title = opts.title or "Inventory",
    count = opts.count or 3,
    cursor = opts.cursor,
    entry = opts.name and {
      text = opts.name,
      category = opts.group or "WEAPONS",
      where = opts.where or "character",
      denial = opts.denial or "",
      enabled = opts.enabled ~= false,
      marked = opts.marked or 0,
    } or nil,
  })
end

check.equal(
  say(screen({ cursor = 1, name = "pocket knife" }), nil),
  "Inventory, 3 entries. / Weapons. / pocket knife, 1 of 3.",
  "Opening says which screen it is and how big it is, then the heading, then the row the cursor already sits on"
)

check.equal(
  say(screen({ cursor = 1, name = "pocket knife" }), screen({ cursor = 1, name = "pocket knife" })),
  "",
  "The same screen redrawn after a key it ignored says nothing, so it cannot talk over itself"
)

check.equal(
  say(screen({ cursor = 2, name = "rock" }), screen({ cursor = 1, name = "pocket knife" })),
  "rock, 2 of 3.",
  "Moving within one heading says the row alone -- name first, then where it sits"
)

check.equal(
  say(screen({ cursor = 3, name = "2 cans of beans", group = "FOOD" }), screen({ cursor = 2, name = "rock" })),
  "Food. / 2 cans of beans, 3 of 3.",
  "Crossing into another heading says the heading before the row, which is the only structure the screen has"
)

check.equal(
  say(
    screen({ cursor = 3, name = "2 cans of beans", group = "FOOD" }),
    screen({ cursor = 4, name = "cola", group = "FOOD" })
  ),
  "2 cans of beans, 3 of 3.",
  "A heading is said when it changes and not on every row under it"
)

check.equal(
  say(
    screen({
      title = "Wield item",
      cursor = 1,
      name = "wheelbarrow",
      denial = "Too heavy to wield",
      enabled = false,
    }),
    nil
  ),
  "Wield item, 3 entries. / Weapons. / wheelbarrow, Too heavy to wield, 1 of 3, unavailable.",
  "A row that cannot be chosen carries the game's own reason, so a key that does nothing is not mistaken for a broken one"
)

check.equal(
  say(
    screen({ title = "Drop items", cursor = 2, name = "rock", marked = 2 }),
    screen({ title = "Drop items", cursor = 2, name = "rock" })
  ),
  "rock, 2 marked, 2 of 3.",
  "Marking answers with the row and its new count, since the mark is drawn and never spoken by the game"
)

check.equal(
  say(screen({ title = "Consume item", count = 0 }), nil),
  "Consume item, no entries.",
  "A screen with nothing on it says so once and names no row, rather than leaving the player to walk an empty list"
)

check.equal(
  say(screen({ count = 1, cursor = 1, name = "rock" }), screen({ count = 3, cursor = 1, name = "rock" })),
  "1 entry. / rock, 1 of 1.",
  "Filtering answers with what is left and then the row, whose position has changed under the cursor without it moving"
)

check.equal(
  say(screen({ title = "Drop items", cursor = 1, name = "rock" }), screen({ cursor = 1, name = "rock" })),
  "Drop items, 3 entries. / Weapons. / rock, 1 of 3.",
  "Another screen over the same rows announces itself, since the title is what says which one has the keyboard"
)

check.equal(
  say(
    screen({ title = "Pick up", count = 6, cursor = 4, name = "rock", where = "map" }),
    screen({ title = "Pick up", count = 6, cursor = 3, name = "pocket knife" })
  ),
  "Weapons, on the ground. / rock, 4 of 6.",
  "Stepping from what the player carries into the pile at their feet says so, which no column and no heading does"
)

check.equal(
  say(
    screen({ title = "Pick up", count = 6, cursor = 5, name = "hammer", where = "map" }),
    screen({ title = "Pick up", count = 6, cursor = 4, name = "rock", where = "map" })
  ),
  "hammer, 5 of 6.",
  "Staying in that pile says the row alone, since where the items are has not changed"
)

check.equal(
  say(screen({ title = "Pick up", cursor = 1, name = "apple", group = "FOOD 3NE", where = "map" }), nil),
  "Pick up, 3 entries. / Food 3 northeast, on the ground. / apple, 1 of 3.",
  "A pile on another square is named the way every other bearing in the layer is, not as the abbreviation the screen draws"
)
