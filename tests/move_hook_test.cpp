#include "avatar.h"
#include "avatar_action.h"
#include "catalua_coord.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "game.h"
#include "lua_hook_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "state_helpers.h"
#include "type_id.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

// These drive a real refused move through avatar_action::move, so the one call
// line inside upstream's avatar_action.cpp is covered rather than assumed.

namespace {

struct seen_refusal {
    int calls = 0;
    std::string obstacle;
    std::optional<tripoint_bub_ms> from;
    std::optional<tripoint_bub_ms> to;
};

// Stands the avatar one square west of the given terrain, so a step east is a
// move the game will refuse.
auto setup_step_into(const ter_id& blocking_terrain)
    -> std::pair<tripoint_bub_ms, tripoint_bub_ms> {
    clear_all_state();
    auto& here = get_map();
    const auto origin = tripoint_bub_ms(60, 60, 0);
    const auto destination = origin + tripoint_rel_ms::east();

    g->place_player(origin);
    here.ter_set(destination, blocking_terrain);
    g->u.moves = 1000;

    return {origin, destination};
}

// Registers a hook that records what the refusal carried, for as long as the
// returned cleanup lives.
auto watch_refusals(const std::shared_ptr<seen_refusal>& seen) -> std::pair<sol::table, int> {
    return test_lua_hooks::push_hook(
        test_lua_hooks::global_lua_state(), "on_player_move_refused", 0, [seen](sol::table params) {
            ++seen->calls;
            seen->obstacle = params["obstacle"].get<sol::optional<std::string>>().value_or("");
            seen->from = cata::detail::lua_coords::as_cpp<tripoint_bub_ms>(
                params["from"].get<sol::object>());
            seen->to = cata::detail::lua_coords::as_cpp<tripoint_bub_ms>(
                params["to"].get<sol::object>());
        });
}

} // namespace

// Walking into a wall is the case the game passes over in silence, and the one
// that sent the player pressing the key harder. The hook must name the wall and
// say which square was refused, since the layer decides on the destination.
TEST_CASE("lua_hook_on_player_move_refused_names_what_is_in_the_way", "[lua]") {
    const auto positions = setup_step_into(ter_id("t_wall"));
    const auto seen = std::make_shared<seen_refusal>();

    const auto [list, idx] = watch_refusals(seen);
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    REQUIRE_FALSE(avatar_action::move(g->u, get_map(), tripoint_rel_ms::east()));

    CHECK(seen->calls == 1);
    // The game's own name for the obstacle, which also covers a vehicle in the
    // way; "plastered wooden wall" at the time of writing.
    CHECK(seen->obstacle.find("wall") != std::string::npos);
    CHECK(seen->from == positions.first);
    CHECK(seen->to == positions.second);
    CHECK(g->u.bub_pos() == positions.first);
}

// A locked door is refused with a message of its own, which reaches Lua through
// on_add_msg. Firing here as well would say the same thing twice, so the call
// sits after every branch that speaks -- and this is what pins it there.
TEST_CASE("lua_hook_on_player_move_refused_is_silent_where_the_game_speaks", "[lua]") {
    setup_step_into(ter_id("t_door_locked"));
    const auto seen = std::make_shared<seen_refusal>();

    const auto [list, idx] = watch_refusals(seen);
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    REQUIRE_FALSE(avatar_action::move(g->u, get_map(), tripoint_rel_ms::east()));

    CHECK(seen->calls == 0);
}

// A move the game allows must not look like a refusal, or every ordinary step
// would speak twice: once as ground and once as an obstacle.
TEST_CASE("lua_hook_on_player_move_refused_does_not_fire_on_a_step_that_works", "[lua]") {
    setup_step_into(ter_id("t_floor"));
    const auto seen = std::make_shared<seen_refusal>();

    const auto [list, idx] = watch_refusals(seen);
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    REQUIRE(avatar_action::move(g->u, get_map(), tripoint_rel_ms::east()));

    CHECK(seen->calls == 0);
}
