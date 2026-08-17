#include "avatar.h"
#include "catalua_coord.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "enums.h"
#include "look_hook.h"
#include "lua_hook_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"

#include <memory>
#include <string>

// The look-around screen answers about one square, and the layer's whole job there
// is to say less than the screen prints. What the firing point owes it is small:
// which square, and how much of it the character can make out. The second is the
// part script cannot work out for itself, so it is the part asserted here.

namespace {

struct seen_square {
    int calls = 0;
    std::string sight;
    bool peeking = false;
    std::string action;
    int x = 0;
};

void record(const std::shared_ptr<seen_square>& out, const sol::table& params) {
    ++out->calls;
    out->sight = params["sight"].get<sol::optional<std::string>>().value_or("");
    out->peeking = params["peeking"].get<sol::optional<bool>>().value_or(false);
    out->action = params["action"].get<sol::optional<std::string>>().value_or("");
    const auto cursor = params["cursor"].get<sol::optional<tripoint_bub_ms>>();
    out->x = cursor ? cursor->x() : -1;
}

} // namespace

// Every visibility the game distinguishes has a token of its own, and none of them
// falls together with another. The panel's own words for these are written into a
// window and never returned, so this mapping is what the layer's wording stands on:
// a case collapsed here would make a lit square and a hidden one read alike.
TEST_CASE("lua_hook_on_look_around_names_every_visibility", "[lua]") {
    CHECK(cata::sight_of(VIS_CLEAR) == "clear");
    CHECK(cata::sight_of(VIS_LIT) == "lit");
    CHECK(cata::sight_of(VIS_DARK) == "dark");
    CHECK(cata::sight_of(VIS_BOOMER) == "blur");
    CHECK(cata::sight_of(VIS_BOOMER_DARK) == "blur_dark");
    CHECK(cata::sight_of(VIS_HIDDEN) == "hidden");
}

// The square the avatar is standing on is lit by the avatar being there, so it
// comes back as clearly visible on any map. That is what makes this assertable
// without arranging light.
TEST_CASE("lua_hook_on_look_around_carries_the_square_under_the_cursor", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_square>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_look_around", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const tripoint_bub_ms here = get_avatar().bub_pos();
    // The screen builds this cache once when it opens, and the firing point reads
    // it rather than rebuilding it per keypress. A test that skips it is asking
    // about whatever light an earlier case left behind, which is how this case
    // passed alone and failed in the suite.
    get_map().update_visibility_cache(here.z());
    cata::fire_on_look_around(here, /*show_window=*/true, /*peeking=*/false, "");

    REQUIRE(seen->calls == 1);
    CHECK(seen->x == here.x());
    CHECK(seen->sight == "clear");
    CHECK_FALSE(seen->peeking);
    CHECK(seen->action.empty());

    cata::fire_on_look_around(here, /*show_window=*/true, /*peeking=*/true, "LEVEL_UP");

    REQUIRE(seen->calls == 2);
    CHECK(seen->peeking);
    CHECK(seen->action == "LEVEL_UP");
}

// The same function is the cursor for marking out a zone and for dragging one, and
// in those shapes the caller draws its own window: there is no panel to speak, and
// while a zone is being dragged there is no single square the cursor is about. A
// firing there would announce a square nobody asked about, on a screen whose own
// wording has not been built.
TEST_CASE("lua_hook_on_look_around_is_silent_without_the_info_panel", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_square>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_look_around", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_look_around(get_avatar().bub_pos(), /*show_window=*/false, false, "");
    CHECK(seen->calls == 0);
}

// The screen waits with a timeout so that the pixel minimap and mouse edge
// scrolling keep working, so it completes rounds with no key pressed at all.
// Firing on those would call into Lua several times a second for a square that has
// not changed.
TEST_CASE("lua_hook_on_look_around_ignores_the_input_layer_sentinels", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_square>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_look_around", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const tripoint_bub_ms here = get_avatar().bub_pos();
    for (const std::string& sentinel :
         {std::string("TIMEOUT"), std::string("ERROR"), std::string("ANY_INPUT")}) {
        cata::fire_on_look_around(here, true, false, sentinel);
    }
    CHECK(seen->calls == 0);

    cata::fire_on_look_around(here, true, false, "LEFT");
    CHECK(seen->calls == 1);
}

// The hook has to exist as a table before any mod can insert into it: a name
// missing from `hook_names` makes every registration fail without a word.
TEST_CASE("lua_hook_on_look_around_is_declared_and_idle_when_nothing_listens", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_look_around"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_look_around"};
    REQUIRE_FALSE(cata::has_hooks("on_look_around"));

    cata::fire_on_look_around(tripoint_bub_ms::zero(), true, false, "");
}
