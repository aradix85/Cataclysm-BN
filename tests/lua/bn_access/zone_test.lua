-- What "where am I" sounds like.
--
-- The place as a whole rather than the square she is on: its name, how big it is,
-- how far it runs from her, and which way there is still something she has never
-- seen. A sighted player has the first three off the screen and the map without
-- noticing, and the fourth from the grey of remembered tiles.
--
-- The decisions live in data/access/lib/zone.lua and are pure: the measurement in,
-- a list of strings out.

local zone = require("../../../data/access/lib/zone")

local function say(report) return table.concat(zone.utterances(zone.state(report)), " / ") end

--- A measurement as the binding returns it. Defaults describe a place three tiles
--- by two, with the character in the middle of it and half of it walked.
local function place(opts)
  return {
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
  }
end

check.equal(
  say(place({})),
  "Subway station, 3 by 2 tiles. / Runs north 10, east 10, south 10, west 10. / Seen all of it.",
  "The place is named and sized, then how far it runs from her, then that there is nothing left to find"
)

check.equal(
  say(place({ rs = 40, ss = 6, rw = 30, sw = 4 })),
  "Subway station, 3 by 2 tiles. / Runs north 10, east 10, south 40, west 30. / Unexplored to the south and west.",
  "Where the place runs further than she has been is said as a direction, which is the thing she can act on"
)

check.equal(
  say(place({ rs = 40, ss = 6 })),
  "Subway station, 3 by 2 tiles. / Runs north 10, east 10, south 40, west 10. / Unexplored to the south.",
  "One direction reads as one direction, without the and"
)

check.equal(
  say(place({ rs = 12, ss = 10 })),
  "Subway station, 3 by 2 tiles. / Runs north 10, east 10, south 12, west 10. / Seen all of it.",
  "A couple of squares past what she has seen is the far wall of the room she is in, not somewhere to go"
)

check.equal(
  say(place({ name = "forest", wide = 1, high = 1, rn = 12, re = 12, rs = 12, rw = 12 })),
  "Forest, 1 tile. / Runs north 12, east 12, south 12, west 12. / Seen all of it.",
  "A place of a single tile says so rather than reading out a multiplication"
)

check.equal(
  say(place({ name = "", rn = 30, sn = 2 })),
  "Here, 3 by 2 tiles. / Runs north 30, east 10, south 10, west 10. / Unexplored to the north.",
  "A place the game names with nothing still answers, since silence would read as a dead key"
)
