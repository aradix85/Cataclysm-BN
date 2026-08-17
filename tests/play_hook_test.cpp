#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "play_hook.h"

#include <memory>

// The main loop is where every screen in the game leads back to, whatever it was,
// so this one firing is what a closed screen is answered by. It carries nothing:
// the moment is the whole message, and what is worth saying about it depends on
// where the player has just come from, which only script knows.

namespace {

struct seen_round {
    int calls = 0;
};

} // namespace

// Fired once per round, and only from the loop that reads a key in play. A
// handler must be able to count on that: the layer speaks here, so a second
// firing inside one round would say the same word twice.
TEST_CASE("lua_hook_on_play_input_fires_once_per_round", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_round>();
    const auto [list, idx] = test_lua_hooks::push_hook(lua, "on_play_input", 0, [seen](sol::table) {
        ++seen->calls;
    });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_play_input();
    CHECK(seen->calls == 1);

    cata::fire_on_play_input();
    CHECK(seen->calls == 2);
}

// The hook has to exist as a table before any mod can insert into it: a name
// missing from `hook_names` makes every registration fail without a word. And
// since this fires for every action the player takes, an unregistered hook must
// build no tables at all.
TEST_CASE("lua_hook_on_play_input_is_declared_and_idle_when_nothing_listens", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_play_input"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_play_input"};
    REQUIRE_FALSE(cata::has_hooks("on_play_input"));

    cata::fire_on_play_input();
}
