-- What the morale screen sounds like.
--
-- The game's own answer to "how am I feeling and why". Until this existed the key
-- that opens it changed the screen and said nothing, so the next keypress went
-- into a screen she did not know was there.
--
-- The decisions live in data/access/lib/morale.lua and are pure: tables in, a list
-- of strings out. The reading model itself is menus.lua and is asserted there.
-- What is asserted here is the part that is this screen's alone: the reading
-- position, which belongs to the layer because the screen has none, and the
-- figures below the list, which are said once and not again.

local morale = require("../../../data/access/lib/morale")

local function say(state, previous) return table.concat(morale.utterances(state, previous), " / ") end

--- The screen as the hook hands it over: every row, every time.
local function screen(opts)
  return {
    title = "Morale",
    total = opts.total or "+12",
    action = opts.action or "",
    rows = opts.rows or {
      { text = "Total positive morale", value = "+20" },
      { text = "Enjoyed a hot meal", value = "70%" },
      { text = "Music", value = "30%" },
      { text = "Total negative morale", value = "-8" },
      { text = "Wet", value = "100%" },
    },
    summary = opts.summary or { { text = "Focus trends towards", value = "100" } },
  }
end

local opening = morale.state(screen({}), nil)

check.equal(
  say(opening, nil),
  "Morale. Total morale +12, 5 entries. / Total positive morale, +20, 1 of 5. / Focus trends towards 100.",
  "Opening names the screen and what it is about, then the first row, then the figures below the list"
)

-- The screen does nothing with an arrow key on a list that fits, so this is the
-- whole of what moving through it is.
local second = morale.state(screen({ action = "DOWN" }), opening)

check.equal(
  say(second, opening),
  "Enjoyed a hot meal, 70%, 2 of 5.",
  "A step down says the row and nothing else: not the screen again, and not the figures again"
)

check.equal(
  say(morale.state(screen({ action = "UP" }), second), second),
  "Total positive morale, +20, 1 of 5.",
  "A step up goes back the way it came"
)

check.equal(
  say(morale.state(screen({ action = "UP" }), opening), opening),
  "Wet, 100%, 5 of 5.",
  "A step up from the first row wraps to the last, so a keypress is never answered by silence"
)

check.equal(
  say(morale.state(screen({ action = "ERROR" }), opening), opening),
  "",
  "A key the screen ignored moves nothing and says nothing"
)

-- Nothing affects her morale, which the screen answers with a sentence instead of
-- a list. The figures below it are still drawn, and pain and the fatigue cap are
-- drawn only where they apply.
local bare = morale.state(
  screen({
    total = "0",
    rows = {},
    summary = {
      { text = "Pain level", value = "-8" },
      { text = "Fatigue Morale Cap", value = "50" },
      { text = "Focus trends towards", value = "90" },
    },
  }),
  nil
)

check.equal(
  say(bare, nil),
  "Morale. Total morale 0, no entries. / Pain level -8, Fatigue Morale Cap 50, Focus trends towards 90.",
  "With nothing affecting her morale the list is empty and the figures below it are still said"
)

-- Coming back from the keybindings screen, which this one opens on the question
-- mark. The state waiting is another screen's, so this is an arrival.
local elsewhere = { category = "KEYBINDINGS", title = "Keybindings", text = "", count = 40, cursor = 3 }

check.equal(
  say(morale.state(screen({ action = "HELP_KEYBINDINGS" }), elsewhere), elsewhere),
  "Morale. Total morale +12, 5 entries. / Total positive morale, +20, 1 of 5. / Focus trends towards 100.",
  "Arriving back from another screen reads as arriving, and the reading starts at the top again"
)
