-- What the screen the game opens on sounds like.
--
-- It is the first screen a player meets, the only way into a world, and the one
-- they are returned to when they leave one, so every keypress on it has to be
-- answered by something. It is also two lists at once -- the row of headings,
-- and the list under the selected heading -- and the whole job of the wording
-- is that it is never in doubt which of the two moved.
--
-- The decisions live in data/access/lib/opening.lua and are pure: tables in,
-- list of strings out. That is what makes them assertable here, with no game
-- running and no NVDA present.

local opening = require("../../../data/access/lib/opening")

--- One firing's utterances as a single string, so that both the content and the
--- count of them are pinned by one comparison.
local function say(state, previous) return table.concat(opening.utterances(state, previous), " / ") end

-- The screen as the game builds it: eight headings along the top, each with the
-- letter that jumps straight to it, and under Load the worlds, which carry no
-- letter because they are reached with the arrow keys.
local function screen(heading, key, at, entry, entry_key, in_list, of_list)
  return opening.state({
    category = "MAIN_MENU",
    heading = { text = heading, key = key, cursor = at, count = 8 },
    entry = entry and { text = entry, key = entry_key, cursor = in_list, count = of_list } or nil,
  })
end

-- A world under Load, with how many characters live in it. Worlds carry no
-- letter of their own; they are reached with the arrow keys.
local load_at = function(entry, saves, in_list, of_list)
  local state = opening.state({
    category = "MAIN_MENU",
    heading = { text = "Load", key = "a", cursor = 3, count = 8 },
    entry = { text = entry, key = "", saves = saves, cursor = in_list, count = of_list },
  })
  return state
end
local new_game_at = function(entry, key, in_list) return screen("New Game", "n", 2, entry, key, in_list, 7) end

check.equal(
  say(load_at("Boston", 2, 1, 3), nil),
  "Main menu, 8 entries. / Load, submenu, a, 3 of 8. / Boston, 2 characters, 1 of 3.",
  "Arriving says the screen, then the heading with its key and that a list hangs under it, then what is selected there"
)

check.equal(
  say(load_at("Boston", 2, 1, 3), load_at("Boston", 2, 1, 3)),
  "",
  "A redraw that changed nothing says nothing, so the screen cannot talk over itself"
)

check.equal(
  say(load_at("Springfield", 1, 2, 3), load_at("Boston", 2, 1, 3)),
  "Springfield, 1 character, 2 of 3.",
  "Moving down the list under a heading speaks that entry alone -- not the heading, and not submenu again"
)

check.equal(
  say(new_game_at("Custom Character", "u", 1), load_at("Boston", 2, 1, 3)),
  "New Game, submenu, n, 2 of 8. / Custom Character, u, 1 of 7.",
  "Moving along the top names the heading and then the entry the cursor already sits on under it"
)

check.equal(
  say(screen("Quit", "q", 8), screen("Credits", "c", 7)),
  "Quit, q, 8 of 8.",
  "A heading with no list under it is not called a submenu, which is how silence after it reads as complete"
)

check.equal(
  say(screen("MOTD", "m", 1), screen("Quit", "q", 8)),
  "MOTD, m, 1 of 8.",
  "Stepping past the last heading lands on the first, and the position it reads out is the whole answer"
)

check.equal(
  say(new_game_at("Custom Character", "u", 1), new_game_at("Defence mode", "d", 7)),
  "Custom Character, u, 1 of 7.",
  "The list under a heading wraps the same way and is answered the same way"
)

check.equal(
  say(new_game_at("Custom Character", "u", 1), screen("Quit", "q", 8)),
  "New Game, submenu, n, 2 of 8. / Custom Character, u, 1 of 7.",
  "Coming from a heading that had no list is not treated as continuing one"
)

check.equal(
  say(load_at("Boston", 2, 1, 3), screen("Load", "a", 3)),
  "Load, submenu, a, 3 of 8. / Boston, 2 characters, 1 of 3.",
  "A heading whose list was empty and now is not says so, since the word submenu is what tells them apart"
)

-- Leaving the screen for a menu or a prompt and coming back is a fresh arrival:
-- main.lua forgets the screen whenever something else takes the keyboard, and
-- without that the screen a player returned to would be answered by silence.
check.equal(
  say(new_game_at("Play Now!", "o", 5), nil),
  "Main menu, 8 entries. / New Game, submenu, n, 2 of 8. / Play Now!, o, 5 of 7.",
  "Coming back to the screen says everything again, wherever the cursor was left"
)

-- The case the game highlights is not always the lower one: it is "N" for New
-- Game and "a" for Load. Both open the heading, and a capital read out as a
-- pitch change or the word "cap" is a difference about nothing.
check.equal(
  say(screen("New Game", "N", 2, "Custom Character", "U", 1, 7), nil),
  "Main menu, 8 entries. / New Game, submenu, n, 2 of 8. / Custom Character, u, 1 of 7.",
  "A key the game wrote as a capital is spoken as a plain letter, at both levels"
)

-- A world nobody lives in yet: made from the World screen, or left behind when a
-- character died and the world was kept. Load lists it like any other, and
-- choosing it answers with a refusal, so the nought is the useful part.
check.equal(
  say(load_at("Ashfield", 0, 3, 3), load_at("Springfield", 1, 2, 3)),
  "Ashfield, no characters, 3 of 3.",
  "A world with nothing in it says so, rather than being found out by pressing Return"
)

check.equal(
  say(load_at("Boston", 2, 1, 3), load_at("Boston", 3, 1, 3)),
  "Boston, 2 characters, 1 of 3.",
  "The count belongs to the world and is spoken when it reads differently, like any other part of an entry"
)
