-- What the game's own keybindings screen sounds like.
--
-- That screen is reachable from 81 input contexts -- the main loop, the
-- inventory, the overmap, trade, and now the screen the game opens on -- and it
-- is also where a key is rebound, so it is the one place a player can find out
-- what a screen can do and change it by ear.
--
-- It is not a uilist, so it has a firing point of its own in C++
-- (src/keybindings_hook.cpp), but it is handed over in the same shape a menu
-- is and read by the same module. That is what these cases pin: the reuse is
-- deliberate, and a second wording for a second kind of list would be a bug.

local menus = require("../../../data/access/lib/menus")

local function say(state, previous) return table.concat(menus.utterances(state, previous), " / ") end

-- One firing, as src/keybindings_hook.cpp builds it: the context whose keys are
-- listed, the screen's own title, how many rows the filter left, the offset the
-- view sits at, and that row's name with its keys in the second column.
local function screen(category, cursor, name, keys, count)
  return menus.state({
    category = category,
    title = "Keybindings",
    text = "",
    count = count or 74,
    cursor = cursor,
    entry = name and { text = name, column = keys } or nil,
  })
end

check.equal(
  say(screen("DEFAULTMODE", 1, "Ask what is around you", "F9"), nil),
  "Keybindings, 74 entries. / Ask what is around you, F9, 1 of 74.",
  "Opening the key list says what it is and how long it is, then the first row: what the action is called and the key that does it"
)

check.equal(
  say(screen("DEFAULTMODE", 2, "Wield an item", "w"), screen("DEFAULTMODE", 1, "Ask what is around you", "F9")),
  "Wield an item, w, 2 of 74.",
  "Scrolling one line answers with the line it scrolled to, and the screen never names itself twice"
)

check.equal(
  say(screen("DEFAULTMODE", 1, "Wield an item", "w", 1), screen("DEFAULTMODE", 1, "Ask what is around you", "F9")),
  "1 entry. / Wield an item, w, 1 of 1.",
  "Typing into the filter says how much is left and then what it left, which is how the rows a short list will not scroll to are reached"
)

check.equal(
  say(screen("DEFAULTMODE", nil, nil, nil, 0), screen("DEFAULTMODE", 1, "Wield an item", "w", 1)),
  "No entries.",
  "A filter that matches nothing says so, instead of leaving the player to wonder whether the keyboard died"
)

-- An action nothing is bound to is drawn in red, which is nothing at all to a
-- player who cannot see it. The C++ side hands over the game's own words for it
-- rather than an empty column, so it is heard rather than inferred from silence.
check.equal(
  say(screen("INVENTORY", 3, "Compare two items", "Unbound globally!"), screen("INVENTORY", 2, "Wield an item", "w")),
  "Compare two items, Unbound globally!, 3 of 74.",
  "An action with no key says that it has none"
)

-- The keybindings screen and the menu underneath it are told apart by the
-- context, since both are titled lists and the menu is still open behind this
-- one. Leaving the key list has to make that menu announce itself again:
-- arriving back somewhere is arriving.
check.equal(
  say(
    menus.state({
      category = "UILIST",
      title = "",
      text = "MAIN MENU",
      count = 16,
      cursor = 1,
      entry = { text = "Options", column = "" },
    }),
    screen("UILIST", 1, "Confirm", "RETURN")
  ),
  "MAIN MENU, 16 entries. / Options, 1 of 16.",
  "Closing the key list returns to the menu, which names itself again rather than answering with silence"
)
