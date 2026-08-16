-- What the game's own keybindings screen sounds like.
--
-- That screen is reachable from 81 input contexts -- the main loop, the
-- inventory, the overmap, trade, and now the screen the game opens on -- and it
-- is the only place that says what a screen can do and lets a key be changed.
--
-- It is not a uilist, so it has a firing point of its own in C++
-- (src/keybindings_hook.cpp), but its wording composes the menu's reading model
-- rather than repeating it. That reuse is deliberate, and a second wording for
-- a second kind of list would be a bug -- which is what these cases pin.

local keybindings = require("../../../data/access/lib/keybindings")
local menus = require("../../../data/access/lib/menus")

local function say(state, previous)
  return table.concat(keybindings.utterances(state, previous), " / ")
end

local KEYS = { add_local = "+", add_global = "=", remove = "-" }

-- One firing, as src/keybindings_hook.cpp builds it: the context whose keys are
-- listed, the screen's own title, how many rows the filter left, the selected
-- row, that row's name with the keys bound to it, and -- only while a change is
-- being made -- the letter that picks it.
local function screen(opts)
  return keybindings.state({
    category = opts.category or "DEFAULTMODE",
    title = "Keybindings",
    count = opts.count or 74,
    cursor = opts.cursor,
    picking = opts.picking,
    keys = KEYS,
    entry = opts.name and { text = opts.name, column = opts.keys, letter = opts.letter } or nil,
  })
end

check.equal(
  say(screen({ cursor = 1, name = "Ask what is around you", keys = "F9" }), nil),
  "Keybindings, 74 entries. / Ask what is around you, F9, 1 of 74. / Press + to add a key here, = to add it everywhere, - to remove one.",
  "Opening says what the screen is, then the selected row, and last how a key is changed -- an instruction ahead of the content is F4"
)

check.equal(
  say(
    screen({ cursor = 2, name = "Wield an item", keys = "w" }),
    screen({ cursor = 1, name = "Ask what is around you", keys = "F9" })
  ),
  "Wield an item, w, 2 of 74.",
  "Stepping one row answers with the row it landed on, and neither the screen's name nor the instruction is heard twice"
)

check.equal(
  say(
    screen({ cursor = 1, name = "Wield an item", keys = "w", count = 1 }),
    screen({ cursor = 1, name = "Ask what is around you", keys = "F9" })
  ),
  "1 entry. / Wield an item, w, 1 of 1.",
  "Typing into the filter says how much is left and then what it left"
)

check.equal(
  say(screen({ count = 0 }), screen({ cursor = 1, name = "Wield an item", keys = "w", count = 1 })),
  "No entries.",
  "A filter that matches nothing says so, instead of leaving the player wondering whether the keyboard died"
)

-- An action nothing is bound to is drawn in red, which is nothing at all to a
-- player who cannot see it. The C++ side hands over the game's own words for it
-- rather than an empty column, so it is heard rather than inferred from silence.
check.equal(
  say(
    screen({ category = "INVENTORY", cursor = 3, name = "Compare two items", keys = "Unbound globally!" }),
    screen({ category = "INVENTORY", cursor = 2, name = "Wield an item", keys = "w" })
  ),
  "Compare two items, Unbound globally!, 3 of 74.",
  "An action with no key says that it has none"
)

-- Pressing the key that starts a change rewrites the row's second column from
-- the key it has to the letter that picks it. Nothing announces the mode, and
-- nothing needs to: the row answers with the thing to press next.
check.equal(
  say(
    screen({ cursor = 2, name = "Wield an item", keys = "w", picking = true, letter = "b" }),
    screen({ cursor = 2, name = "Wield an item", keys = "w" })
  ),
  "Wield an item, b, 2 of 74.",
  "Starting a change answers with the letter that picks the selected row, the only place that letter is ever said"
)

check.equal(
  say(
    screen({ cursor = 3, name = "Close the window", keys = "ESC", picking = true, letter = "c" }),
    screen({ cursor = 2, name = "Wield an item", keys = "w", picking = true, letter = "b" })
  ),
  "Close the window, c, 3 of 74.",
  "Moving while a change is being made keeps answering with the letter, since that is what the next keypress is about"
)

-- The keybindings screen and the menu underneath it are told apart by the
-- context, since both are titled lists and the menu is still open behind this
-- one. Leaving the key list has to make that menu announce itself again:
-- arriving back somewhere is arriving.
check.equal(
  table.concat(
    menus.utterances(
      menus.state({
        category = "UILIST",
        title = "",
        text = "MAIN MENU",
        count = 16,
        cursor = 1,
        entry = { text = "Options", column = "" },
      }),
      screen({ category = "UILIST", cursor = 1, name = "Confirm", keys = "RETURN" })
    ),
    " / "
  ),
  "MAIN MENU, 16 entries. / Options, 1 of 16.",
  "Closing the key list returns to the menu, which names itself again rather than answering with silence"
)
