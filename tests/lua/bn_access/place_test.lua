-- What walking and asking sound like, now that a place has a name in play.
--
-- The game names the region a square belongs to -- "overgrown cabin", "forest" --
-- and says it nowhere at all outside the two screens that draw a map. So the only
-- way to learn that a walk has arrived somewhere was to open the overmap and come
-- back, which is a screen round trip for a fact the game already knows.
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

check.equal(
  say(surroundings.overview({ area = "forest", enemies = {}, others = {}, landmarks = {} })),
  "Forest. / Nothing nearby.",
  "Asking what is around says where she is first, and still owes the empty word for the question itself"
)

check.equal(
  say(surroundings.overview({
    area = "overgrown cabin",
    enemies = { { name = "zombie", dx = 3, dy = -3 } },
    others = {},
    landmarks = {},
  })),
  "Overgrown cabin. / 1 enemy. zombie, 3 northeast.",
  "The region comes before the enemies, and the enemies still come before everything else"
)

check.equal(
  say(surroundings.overview({ enemies = {}, others = {}, landmarks = {} })),
  "Nothing nearby.",
  "With no region given the answer is what it always was, so a caller that cannot ask still gets one"
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
  say(surroundings.overview({ area = "subway", reach = reach(3, 7, 0, 2) })),
  "Subway. / Space: north 3, east 7, south blocked, west 2. / Nothing nearby.",
  "The room is read as a fixed compass sweep, so a corridor running east is hearable as one"
)

check.equal(
  say(surroundings.overview({ reach = reach(nil, nil, 1, 1) })),
  "Space: north open, east open, south 1, west 1. / Nothing nearby.",
  "A direction nothing stopped her in is a different fact from a long corridor, and is said as one"
)

check.equal(
  say(surroundings.overview({
    area = "field",
    reach = reach(2, 2, 2, 2),
    enemies = { { name = "zombie", dx = 0, dy = -3 } },
  })),
  "Field. / Space: north 2, east 2, south 2, west 2. / 1 enemy. zombie, 3 north.",
  "Where she is comes first and what is in it after, and the empty word is not owed once something was found"
)
