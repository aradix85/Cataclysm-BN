-- Bearings and the surroundings overview, against the modules the mod loads.
-- Assertions live on the C++ side.

local bearing = require("../../data/mods/bn_access/lib/bearing")
local surroundings = require("../../data/mods/bn_access/lib/surroundings")

-- All eight points. x runs east, y runs south, so north is negative y.
test_data.east = bearing.of(3, 0)
test_data.northeast = bearing.of(3, -3)
test_data.north = bearing.of(0, -3)
test_data.northwest = bearing.of(-3, -3)
test_data.west = bearing.of(-3, 0)
test_data.southwest = bearing.of(-3, 3)
test_data.south = bearing.of(0, 3)
test_data.southeast = bearing.of(3, 3)

-- Snapped by angle, not by the sign of each axis: a long approach that drifts
-- one square sideways must not wobble between two words.
test_data.mostly_east = bearing.of(10, 1)
test_data.zero = tostring(bearing.of(0, 0))

-- A diagonal counts as one step, as it does when walking.
test_data.diagonal_distance = bearing.distance(3, -3)
test_data.described = bearing.describe(4, -4)

-- Enemies first, one utterance per group, nearest named (P2, P4).
test_data.overview = table.concat(
  surroundings.overview({
    enemies = {
      { name = "zombie", dx = 4, dy = -4 },
      { name = "zombie dog", dx = 9, dy = 0 },
      { name = "skeleton", dx = 2, dy = 2 },
    },
    others = { { name = "rabbit", dx = 6, dy = 6 } },
    landmarks = { { name = "closed wood door", dx = 3, dy = 0 } },
  }),
  " / "
)

-- One of something reads as one, not as a count with a nearest.
test_data.single = table.concat(surroundings.overview({ enemies = { { name = "zombie", dx = 0, dy = -2 } } }), " / ")

-- Asked and answered: an empty field says so rather than staying silent, which
-- cannot be told apart from a key that never arrived.
test_data.empty = table.concat(surroundings.overview({}), " / ")
