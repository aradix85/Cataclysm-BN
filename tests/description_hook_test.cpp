#include "avatar.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "description_hook.h"
#include "lua_hook_helpers.h"
#include "map_helpers.h"
#include "player_helpers.h"

#include <memory>
#include <string>

// The detail screen behind the look-around cursor's describe key. Its own three
// keys swap which of the three things on a square is described, and the firing point
// owes script two facts: which of them is showing, and that thing's own description
// -- or nothing at all, which is what makes "no creature here" sayable without the
// layer guessing why.

namespace {

struct seen_description {
    int calls = 0;
    std::string target;
    std::string text;
    std::string signage;
};

void record(const std::shared_ptr<seen_description>& out, const sol::table& params) {
    ++out->calls;
    out->target = params["target"].get<sol::optional<std::string>>().value_or("");
    out->text = params["text"].get<sol::optional<std::string>>().value_or("");
    out->signage = params["signage"].get<sol::optional<std::string>>().value_or("");
}

} // namespace

// The terrain of a square always describes itself, on any map, which is what makes
// this assertable without arranging anything. A creature is the opposite: an empty
// square has none, and an empty text rather than a sentence of the screen's own is
// what the layer's wording stands on.
TEST_CASE("lua_hook_on_description_carries_what_the_screen_is_describing", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_description>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_description", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const tripoint_bub_ms here = get_avatar().bub_pos();

    cata::fire_on_description(here, "terrain", "");
    REQUIRE(seen->calls == 1);
    CHECK(seen->target == "terrain");
    CHECK_FALSE(seen->text.empty());
    CHECK(seen->signage.empty());

    // The avatar is standing here, so nothing else is: the creature the screen would
    // describe is absent and the answer is an empty one.
    cata::fire_on_description(here + tripoint_rel_ms(1, 0, 0), "creature", "CREATURE");
    REQUIRE(seen->calls == 2);
    CHECK(seen->target == "creature");
    CHECK(seen->text.empty());
}

// The screen reads keys in a loop of its own and answers only three of them, so it
// completes rounds on keys it ignored. Firing on the input layer's sentinels would
// rebuild a description nobody asked for.
TEST_CASE("lua_hook_on_description_ignores_the_input_layer_sentinels", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_description>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_description", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const tripoint_bub_ms here = get_avatar().bub_pos();
    for (const std::string& sentinel :
         {std::string("TIMEOUT"), std::string("ERROR"), std::string("ANY_INPUT")}) {
        cata::fire_on_description(here, "terrain", sentinel);
    }
    CHECK(seen->calls == 0);

    cata::fire_on_description(here, "terrain", "FURNITURE");
    CHECK(seen->calls == 1);
}

// The hook has to exist as a table before any mod can insert into it: a name
// missing from `hook_names` makes every registration fail without a word.
TEST_CASE("lua_hook_on_description_is_declared_and_idle_when_nothing_listens", "[lua]") {
    clear_avatar();
    clear_map();

    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_description"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_description"};
    REQUIRE_FALSE(cata::has_hooks("on_description"));

    cata::fire_on_description(get_avatar().bub_pos(), "terrain", "");
}
