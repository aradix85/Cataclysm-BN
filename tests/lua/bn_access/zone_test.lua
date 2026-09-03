-- What the place she is standing in sounds like, as the heading and the last frame
-- line of the one list that answers "where am I".
--
-- Its name and how big it is are the heading the list opens with; which way there
-- is still something she has never seen is a row in it. How far the place runs is
-- deliberately not said: that was a second compass sweep beside the one saying how
-- far the room lets her walk, and two sweeps in one breath is what made one
-- question read as several systems.
--
-- The decisions live in data/access/lib/zone.lua and are pure: the measurement in,
-- strings out.

local zone = require("../../../data/access/lib/zone")

--- A measurement as the binding returns it. Defaults describe a place three tiles
--- by two, with the character in the middle of it and half of it walked.
local function place(opts)
  return zone.state({
    name = opts.name or "subway station",
    tiles_wide = opts.wide or 3,
    tiles_high = opts.high or 2,
    reach_north = opts.rn or 10,
    reach_east = opts.re or 10,
    reach_south = opts.rs or 10,
    reach_west = opts.rw or 10,
    seen_north = opts.sn or 10,
    seen_east = opts.se or 10,
    seen_south = opts.ss or 10,
    seen_west = opts.sw or 10,
  })
end

check.equal(
  zone.title(place({})),
  "Subway station, 3 by 2 tiles",
  "The heading names the place and its size, and ends without a full stop because the row count follows it"
)

check.equal(
  zone.title(place({ name = "forest", wide = 1, high = 1 })),
  "Forest, 1 tile",
  "A place of a single tile says so rather than reading out a multiplication"
)

check.equal(
  zone.title(place({ name = "" })),
  "Here, 3 by 2 tiles",
  "A place the game names with nothing still answers, since silence would read as a dead key"
)

check.equal(
  zone.unexplored(place({})),
  "Seen all of it.",
  "A place she has walked the whole of says so, rather than leaving her to wonder"
)

check.equal(
  zone.unexplored(place({ rs = 40, ss = 6, rw = 30, sw = 4 })),
  "Unexplored to the south and west.",
  "Where the place runs further than she has been is said as a direction, which is the thing she can act on"
)

check.equal(
  zone.unexplored(place({ rs = 40, ss = 6 })),
  "Unexplored to the south.",
  "One direction reads as one direction, without the and"
)

check.equal(
  zone.unexplored(place({ rs = 12, ss = 10 })),
  "Seen all of it.",
  "A couple of squares past what she has seen is the far wall of the room she is in, not somewhere to go"
)
