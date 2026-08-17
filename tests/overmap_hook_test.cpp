#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "overmap_hook.h"

#include <memory>
#include <string>

// An overmap refuses to exist without a world, so these drive the firing
// function with the tile the screen hands it. What that costs is the single call
// line in src/overmap_ui.cpp; what it buys is that everything the screen knows
// about a place and never says out loud is asserted here rather than only at a
// keyboard.

namespace {

struct seen_tile {
    int calls = 0;
    std::string place;
    std::string note;
    bool seen = false;
    bool explored = false;
    int dx = 0;
    int dy = 0;
    int dz = 0;
    int route = 0;
    std::string action;
};

// Reads one firing of the hook out of the params table.
void record(const std::shared_ptr<seen_tile>& out, const sol::table& params) {
    ++out->calls;
    out->place = params["place"].get<sol::optional<std::string>>().value_or("");
    out->note = params["note"].get<sol::optional<std::string>>().value_or("");
    out->seen = params["seen"].get<sol::optional<bool>>().value_or(false);
    out->explored = params["explored"].get<sol::optional<bool>>().value_or(false);
    out->dx = params["dx"].get<sol::optional<int>>().value_or(-1);
    out->dy = params["dy"].get<sol::optional<int>>().value_or(-1);
    out->dz = params["dz"].get<sol::optional<int>>().value_or(-1);
    out->route = params["route"].get<sol::optional<int>>().value_or(-1);
    out->action = params["action"].get<sol::optional<std::string>>().value_or("");
}

cata::overmap_view a_house() {
    cata::overmap_view view;
    view.place = "house in central Springfield";
    view.note = "gun shop";
    view.seen = true;
    view.explored = true;
    view.dx = 4;
    view.dy = -4;
    view.dz = -1;
    view.route = 12;
    view.action = "CHOOSE_DESTINATION";
    return view;
}

} // namespace

// The tile has to arrive whole: what the place is called, where it is relative to
// the character, and the three things the screen draws as colour alone -- whether
// it was ever seen, whether it has been explored, and whether a route leads
// there. Every one of them is silence in the game as it stands.
TEST_CASE("lua_hook_on_overmap_carries_the_tile_under_the_cursor", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_tile>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_overmap", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_overmap(a_house());

    CHECK(seen->calls == 1);
    CHECK(seen->place == "house in central Springfield");
    CHECK(seen->note == "gun shop");
    CHECK(seen->seen);
    CHECK(seen->explored);
    CHECK(seen->dx == 4);
    CHECK(seen->dy == -4);
    CHECK(seen->dz == -1);
    CHECK(seen->route == 12);
    CHECK(seen->action == "CHOOSE_DESTINATION");
}

// A tile the character has never seen carries no description at all, which is
// what the sidebar draws as "Unexplored". Handing over an empty place rather than
// the terrain's own name is the whole point: the map is a memory, and speaking
// what is on a tile nobody has visited would report what the character cannot
// know.
TEST_CASE("lua_hook_on_overmap_reports_an_unseen_tile_as_having_no_place", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_tile>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_overmap", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::overmap_view view;
    view.dx = 9;
    view.dy = 9;

    cata::fire_on_overmap(view);

    REQUIRE(seen->calls == 1);
    CHECK(seen->place.empty());
    CHECK_FALSE(seen->seen);
    CHECK_FALSE(seen->explored);
    CHECK(seen->route == 0);
    CHECK(seen->action.empty());
}

// The hook has to exist as a table before any mod can insert into it: a name
// missing from `hook_names` makes every registration fail without a word. And
// the overmap completes a round several times a second so that it can blink, so
// an unregistered hook must build no tables at all.
TEST_CASE("lua_hook_on_overmap_is_declared_and_idle_when_nothing_listens", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_overmap"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_overmap"};
    REQUIRE_FALSE(cata::has_hooks("on_overmap"));

    cata::fire_on_overmap(a_house());
}
