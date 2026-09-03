-- What walking and asking sound like, now that a place has a name in play.
--
-- The game names the region a square belongs to -- "overgrown cabin", "forest" --
-- and says it nowhere at all outside the two screens that draw a map. So the only
-- way to learn that a walk has arrived somewhere was to open the overmap and come
-- back, which is a screen round trip for a fact the game already knows.
--
-- Asking is the frame of the room and nothing else: whether she can see, and how
-- far each way lets her walk. What is standing and lying about is the game's own
-- list on its own key, and the ways out are rows under this frame -- see
-- exits_test.lua.
--
-- Both modules are pure: a table in, a list of strings out.

local movement = require("../../../data/access/lib/movement")
local surroundings = require("../../../data/access/lib/surroundings")

local function say(lines) return table.concat(lines, " / ") end

check.equal(
  say(movement.utterances({ name = "floor", changed = false, area = "overgrown cabin" })),
  "Overgrown cabin.",
  "Crossing into another region says the region, even when the ground underfoot did not change"
)

check.equal(
  say(movement.utterances({ name = "wooden floor", changed = true, area = "overgrown cabin" })),
  "Overgrown cabin. / wooden floor.",
  "Region first and ground second, because the region is the larger of the two answers"
)

check.equal(
  say(movement.utterances({ name = "grass", changed = false, area = "" })),
  "",
  "A step inside one region across unchanged ground stays silent, which is nearly every step"
)

check.equal(
  say(movement.utterances({ blocked = true, name = "wall", area = "overgrown cabin" })),
  "wall, blocked.",
  "A refused step names what stopped it and nothing else: no step was taken, so no region was entered"
)

--- The room as the layer measures it: how far each way before something stops her.
local function reach(n, e, s, w)
  local function arm(v)
    if v == nil then return { steps = 12, blocked = false } end
    return { steps = v, blocked = true }
  end
  return { north = arm(n), east = arm(e), south = arm(s), west = arm(w) }
end

check.equal(
  say(surroundings.space({ reach = reach(3, 7, 0, 2) })),
  "Space: north 3, east 7, south blocked, west 2.",
  "The room is read as a fixed compass sweep, so a corridor running east is hearable as one"
)

check.equal(
  say(surroundings.space({ reach = reach(nil, nil, 1, 1) })),
  "Space: north open, east open, south 1, west 1.",
  "A direction nothing stopped her in is a different fact from a long corridor, and is said as one"
)

check.equal(
  say(surroundings.space({
    reach = {
      north = { steps = 4, blocked = false, unknown = true },
      east = { steps = 2, blocked = true },
      south = { steps = 0, blocked = true },
      west = { steps = 12, blocked = false },
    },
  })),
  "Space: north 4 or more, east 2, south blocked, west open.",
  "A direction she has not been far enough along to know is said as a floor, not as open ground"
)

check.equal(
  say(surroundings.space({ dark = true, reach = reach(1, 1, 1, 1) })),
  "Dark. / Space: north 1, east 1, south 1, west 1.",
  "An unlit place is said before the room is measured in it, because it is why every arm stops one square out"
)

check.equal(
  say(surroundings.space({ reach = reach(3, 3, 3, 3) })),
  "Space: north 3, east 3, south 3, west 3.",
  "Where there is light nothing is said about light, since a lit room is the ordinary case"
)

check.equal(
  say(surroundings.space({ dark = true })),
  "Dark.",
  "The darkness is owed even where there is no room measurement to explain, which is how a caller with no reach still hears it"
)

check.equal(
  say(surroundings.space({})),
  "",
  "A lit room nothing was measured in says nothing at all, and the list it heads is what answers instead"
)
