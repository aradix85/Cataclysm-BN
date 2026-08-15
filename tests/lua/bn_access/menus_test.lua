-- What every menu in the game sounds like.
--
-- One hook carries all of them -- the menu ESC opens, the action menu on
-- RETURN, the item action and examine menus, the vehicle controls -- so this
-- wording is not one screen's. It is also the reading model every later list
-- reuses, which is why it is pinned entry by entry rather than sampled.
--
-- The decisions live in data/access/lib/menus.lua and are pure: tables
-- in, list of strings out. That is what makes them assertable here, with no
-- game running and no NVDA present.

local menus = require("../../../data/access/lib/menus")

--- One firing's utterances as a single string, so that both the content and the
--- count of them are pinned by one comparison.
local function say(state, previous) return table.concat(menus.utterances(state, previous), " / ") end

-- The menu ESC opens, as the game builds it: no title in the border, its name
-- in the header above the list (src/action.cpp, handle_main_menu).
local main_menu = function(cursor, name, count)
  return menus.state({
    category = "UILIST",
    title = "",
    text = "MAIN MENU",
    count = count or 16,
    cursor = cursor,
    entry = name and { text = name, column = "", enabled = true } or nil,
  })
end

check.equal(
  say(main_menu(1, "Options"), nil),
  "MAIN MENU, 16 entries. / Options, 1 of 16.",
  "Opening says what the menu is and how big it is, then where the cursor already sits -- never the list"
)

check.equal(
  say(main_menu(1, "Options"), main_menu(1, "Options")),
  "",
  "The same menu redrawn after a key it ignored says nothing, so it cannot talk over itself"
)

check.equal(
  say(main_menu(2, "Save"), main_menu(1, "Options")),
  "Save, 2 of 16.",
  "A moved selection is spoken, and the menu never names itself twice"
)

check.equal(
  say(main_menu(1, "Options"), main_menu(16, "Reload Lua")),
  "Options, 1 of 16.",
  "Stepping past the last entry lands on the first, and the position it reads out is the whole answer"
)

check.equal(
  say(main_menu(16, "Reload Lua"), main_menu(1, "Options")),
  "Reload Lua, 16 of 16.",
  "And stepping before the first lands on the last, with nothing said about the jump"
)

check.equal(
  say(main_menu(1, "Options", 4), main_menu(16, "Reload Lua")),
  "4 entries. / Options, 1 of 4.",
  "A filter that shrank the list says how much is left, and then where the cursor now sits"
)

-- The game's own action menu, the one that lists every verb the player has and
-- this mod's commands with them.
check.equal(
  say(
    menus.state({
      category = "UILIST",
      title = "",
      text = "Actions",
      count = 12,
      cursor = 3,
      entry = { text = "Look\226\128\166", column = "", enabled = true },
    }),
    nil
  ),
  "Actions, 12 entries. / Look, submenu, 3 of 12.",
  "An entry that descends into another menu is marked only by an ellipsis, which a synthesiser does not say"
)

-- A menu with a title in the border and no header above the list, which is the
-- other way the game builds one.
local technique = function(cursor, name, column, enabled)
  return menus.state({
    category = "UILIST",
    title = "Choose a style",
    text = "",
    count = 7,
    cursor = cursor,
    entry = { text = name, column = column, enabled = enabled },
  })
end

check.equal(
  say(technique(4, "Brawling", "<color_light_blue>Level 3</color>", true), nil),
  "Choose a style, 7 entries. / Brawling, Level 3, 4 of 7.",
  "The second column is drawn on the entry's own row and is spoken with it; colour markup is drawn, not said"
)

check.equal(
  say(technique(5, "Niten Ichi-Ryu", "", false), technique(4, "Brawling", "", true)),
  "Niten Ichi-Ryu, 5 of 7, unavailable.",
  "An entry the game will refuse says so, after its position"
)

check.equal(
  say(main_menu(1, "Save", 2), main_menu(1, "Options")),
  "2 entries. / Save, 1 of 2.",
  "Typing into the filter is answered by how much is left"
)

check.equal(
  say(main_menu(nil, nil, 0), main_menu(1, "Options")),
  "No entries.",
  "A filter that matches nothing says so, which is otherwise only discoverable by walking a list that is gone"
)

check.equal(
  say(main_menu(3, "Safe mode: on"), main_menu(3, "Safe mode: off")),
  "Safe mode: on, 3 of 16.",
  "A toggle rewrites itself where it stands: what is spoken is what reads differently, not only what moved"
)

check.equal(
  say(main_menu(7, "   "), main_menu(6, "Colors")),
  "Blank, 7 of 16.",
  "A separator has no text, and a keypress answered by silence cannot be told from a key that never arrived"
)

check.equal(
  say(main_menu(1, "Options"), nil),
  "MAIN MENU, 16 entries. / Options, 1 of 16.",
  "Closing a menu and opening the same one again says everything again"
)

-- A list of two, where the cursor is always at one end or the other.
local pair_menu = function(cursor, name)
  return menus.state({
    category = "UILIST",
    title = "Really quit?",
    text = "",
    count = 2,
    cursor = cursor,
    entry = { text = name, column = "", enabled = true },
  })
end

check.equal(
  say(pair_menu(2, "No"), pair_menu(1, "Yes")),
  "No, 2 of 2.",
  "A two-entry list reads like any other: the entry and where it sits, and nothing about its ends"
)

-- An entry that descends without an ellipsis to show it. A uilist marks a
-- submenu that way and the opening screen does not mark it at all, so the flag
-- exists for a caller that knows what the drawing does not say.
check.equal(
  say(
    menus.state({
      category = "UILIST",
      title = "",
      text = "MAIN MENU",
      count = 16,
      cursor = 2,
      entry = { text = "Options", column = "", opens = true, enabled = true },
    }),
    nil
  ),
  "MAIN MENU, 16 entries. / Options, submenu, 2 of 16.",
  "A caller that knows an entry descends says so, and it reads exactly as an ellipsis would"
)
