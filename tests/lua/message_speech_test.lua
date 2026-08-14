-- Drives the message policy and the scrollback from Lua, against the modules
-- the mod itself loads. Assertions live on the C++ side; each case writes one
-- string into test_data, with " / " between utterances so that both content and
-- count are pinned by a single comparison.

local messages = require("../../data/mods/bn_access/lib/messages")
local scrollback = require("../../data/mods/bn_access/lib/scrollback")
local text = require("../../data/mods/bn_access/lib/text")

-- Decoration is drawn, not said. A line with no letter and no digit left would
-- otherwise be read out glyph by glyph.
test_data.speaks_prose = tostring(messages.should_speak(MsgType.neutral, "You hit the zombie."))
test_data.speaks_border = tostring(messages.should_speak(MsgType.neutral, text.clean("--\\ // --")))
test_data.speaks_debug = tostring(messages.should_speak(MsgType.debug, "Spawned 3 monsters."))

-- Taking damage must not wait behind a list of things lying on the floor (P7).
test_data.urgency_bad = messages.urgency(MsgType.bad)
test_data.urgency_warning = messages.urgency(MsgType.warning)
test_data.urgency_info = messages.urgency(MsgType.info)
test_data.urgency_good = messages.urgency(MsgType.good)

-- A fight logs the same miss over and over. Consecutive repeats become one
-- entry with a count, so a bounded buffer is not flushed by one bad turn -- and
-- the count is spoken, because three misses is different news from one.
local buffer = scrollback.new(3)
scrollback.add(buffer, "You miss the zombie.")
scrollback.add(buffer, "You miss the zombie.")
scrollback.add(buffer, "You miss the zombie.")
test_data.repeats = scrollback.current(buffer)

-- Paging back one entry at a time, and a word at each end rather than a silent
-- wrap. nil is what the caller turns into that word.
scrollback.add(buffer, "The zombie hits you.")
test_data.after_new = scrollback.current(buffer)
test_data.one_back = scrollback.older(buffer)
test_data.past_oldest = tostring(scrollback.older(buffer))
test_data.one_forward = scrollback.newer(buffer)
test_data.past_newest = tostring(scrollback.newer(buffer))

-- The buffer is bounded and drops the oldest first.
local small = scrollback.new(2)
scrollback.add(small, "one")
scrollback.add(small, "two")
scrollback.add(small, "three")
scrollback.older(small)
test_data.dropped_oldest = scrollback.current(small)
test_data.nothing_older = tostring(scrollback.older(small))

-- Nothing heard yet is a state the caller has to be able to tell apart.
test_data.empty_is_empty = tostring(scrollback.is_empty(scrollback.new()))
test_data.empty_current = tostring(scrollback.current(scrollback.new()))
