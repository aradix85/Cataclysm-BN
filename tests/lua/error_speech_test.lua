-- Drives the error-report wording against the module the mod itself loads.
-- Assertions live on the C++ side; each case writes its result into test_data
-- as one string, with " / " between utterances so that both the content and the
-- count are pinned by one comparison.

local errors = require("../../data/access/lib/errors")

local function say(current, previous) return table.concat(errors.utterances(current, previous), " / ") end

-- An ordinary report from the game: one sentence, already punctuated.
test_data.plain = say("Attempted to load unknown item id.", nil)

-- A fault in Lua, which is what this hook exists for. Only the first line is
-- the fault; the traceback under it is where it happened, and no amount of
-- listening turns that into an action.
test_data.lua_fault = say(
  "[string \"main.lua\"]:41: attempt to index a nil value (field 'to')\n"
    .. "stack traceback:\n"
    .. "\t[C]: in function 'index'\n"
    .. '\t[string "main.lua"]:41: in function <main.lua:39>',
  nil
)

-- The game shows the same report again once it has repeated too often, and a
-- fault in a per-turn hook repeats every turn. Reading it out again is F5.
local repeated = "Attempted to load unknown item id."
test_data.repeated = say(repeated, repeated)

-- A different error after one that repeated is a new one, and is read in full.
test_data.different = say("Monster spawn failed.", repeated)

-- The screen holds the keyboard whatever it has to say, so a report with no
-- speakable text still has to name itself and the way out. Silence here would
-- be the dead keyboard this whole step is about.
test_data.empty = say("", nil)
test_data.decoration = say("---------", nil)

-- Colour markup reaches the report the same way it reaches a message.
test_data.markup = say("<color_red>Save file is corrupt.</color>", nil)

-- A first line long enough to run away is cut at a word boundary, and says so,
-- because the log has the rest and the player cannot ask for it here.
local long = errors.utterances(("wandering herd placement failed at overmap tile "):rep(8), nil)
test_data.long = long[1]
test_data.long_length = #long[1]
