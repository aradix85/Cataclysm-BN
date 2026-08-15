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

-- The screen as the game builds it: eight headings along the top, and under
-- Load the worlds with the number of characters in each.
local function screen(heading, at, entry, in_list, of_list)
  return opening.state({
    category = "MAIN_MENU",
    heading = { text = heading, cursor = at, count = 8 },
    entry = entry and { text = entry, cursor = in_list, count = of_list } or nil,
  })
end

check.equal(
  say(screen("Load", 3, "Boston (2)", 1, 2), nil),
  "Main menu, 8 entries. / Load, 3 of 8. / Boston (2), 1 of 2.",
  "Arriving says what the screen is, which heading the cursor sits on, and what is selected under it"
)

check.equal(
  say(screen("Load", 3, "Boston (2)", 1, 2), screen("Load", 3, "Boston (2)", 1, 2)),
  "",
  "A redraw that changed nothing says nothing, so the screen cannot talk over itself"
)

check.equal(
  say(screen("Load", 3, "Springfield (1)", 2, 2), screen("Load", 3, "Boston (2)", 1, 2)),
  "Springfield (1), 2 of 2.",
  "Moving down the list under a heading speaks that entry alone, never the heading again"
)

check.equal(
  say(screen("New Game", 2, "Custom Character", 1, 7), screen("Load", 3, "Boston (2)", 1, 2)),
  "New Game, 2 of 8. / Custom Character, 1 of 7.",
  "Moving along the top names the heading and then the first entry under it, and names neither twice"
)

check.equal(
  say(screen("Quit", 8, nil), screen("Credits", 7, nil)),
  "Quit, 8 of 8.",
  "A heading with no list under it is complete on its own, and does not wait for a second sentence"
)

check.equal(
  say(screen("MOTD", 1, nil), screen("Quit", 8, nil)),
  "Back to the top. / MOTD, 1 of 8.",
  "Stepping past the last heading lands on the first, and says so rather than appearing to go on forever"
)

check.equal(
  say(screen("New Game", 2, "Custom Character", 1, 7), screen("New Game", 2, "Defence mode", 7, 7)),
  "Back to the top. / Custom Character, 1 of 7.",
  "The list under a heading wraps the same way and is answered the same way"
)

check.equal(
  say(screen("New Game", 2, "Custom Character", 1, 7), screen("Quit", 8, nil)),
  "New Game, 2 of 8. / Custom Character, 1 of 7.",
  "Coming from a heading that had no list is not treated as continuing one"
)

check.equal(
  say(screen("Load", 3, "Boston (2)", 1, 2), screen("Load", 3, nil)),
  "Boston (2), 1 of 2.",
  "A list that was empty and now is not is entered rather than continued, so its name is not repeated"
)

-- Leaving the screen for a menu or a prompt and coming back is a fresh arrival:
-- main.lua forgets the screen whenever something else takes the keyboard, and
-- without that the screen a player returned to would be answered by silence.
check.equal(
  say(screen("New Game", 2, "Play Now!", 5, 7), nil),
  "Main menu, 8 entries. / New Game, 2 of 8. / Play Now!, 5 of 7.",
  "Coming back to the screen says everything again, wherever the cursor was left"
)
