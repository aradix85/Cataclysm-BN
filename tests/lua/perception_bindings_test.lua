-- Every perception query, called from Lua exactly as the layer will call it.
--
-- The point is coverage of the surface, not of the values: a binding that is
-- missing or misnamed fails here rather than in a silent gap in what the player
-- is told, which is how infrared goggles stayed invisible in CDDA.

local here = gapi.get_map()
local you = gapi.get_avatar()
local at = you:get_pos_ms()

test_data.move_cost = perception.move_cost_at(at)
test_data.has_floor = tostring(perception.has_floor_at(at))
test_data.coverage = perception.coverage_at(at)
test_data.block_chance = perception.block_unaimed_chance_at(at)
test_data.signage = perception.signage_at(at)
test_data.ter_description = perception.ter_description_at(at)
test_data.furn_description = perception.furn_description_at(at)
test_data.sound = perception.sound_at(at)
test_data.footsteps = #perception.footstep_markers()

-- The region a square belongs to. Named at overmap scale and asked at map scale,
-- because the projection between the two is the binding's job: a coordinate of
-- overmap scale is not something script can hand back.
test_data.area_name = perception.area_name_at(at)

-- The special senses take a character and a creature together, which is why
-- they are free functions rather than methods on either.
test_data.infrared_self = tostring(perception.sees_with_infrared(you, you))
test_data.specials_self = tostring(perception.sees_with_specials(you, you))
test_data.describe_infrared = #perception.describe_infrared(you)
test_data.describe_specials = #perception.describe_specials(you)

-- The square the avatar is standing on must be passable and must have a floor,
-- or it could not be standing there. That makes these two assertable without
-- knowing anything about the test map.
test_data.standing_is_passable = tostring(perception.move_cost_at(at) > 0)
