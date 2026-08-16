#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "world_loading_hook.h"

#include <memory>
#include <string>

// The call line itself sits at the top of game::setup, which reads the whole mod
// list and generates a map; a test cannot drive it, so these cover the firing
// function and the shape of what it hands over. What the call line has to keep
// true is stated where it sits: it comes before the work, not after it.

namespace {

struct seen_load {
    int calls = 0;
    std::string world;
    bool reading_data = false;
};

auto watch_loads(const std::shared_ptr<seen_load>& seen) -> std::pair<sol::table, int> {
    return test_lua_hooks::push_hook(
        test_lua_hooks::global_lua_state(), "on_world_loading", 0, [seen](sol::table params) {
            ++seen->calls;
            seen->world = params["world"].get<sol::optional<std::string>>().value_or("");
            seen->reading_data = params["reading_data"].get<sol::optional<bool>>().value_or(false);
        });
}

} // namespace

// The world's own Lua state does not exist yet when this fires -- it is built by
// the load being announced -- so the name has to travel as a plain string rather
// than as a world object a handler could ask questions of.
TEST_CASE("lua_hook_on_world_loading_names_the_world", "[lua]") {
    const auto seen = std::make_shared<seen_load>();

    const auto [list, idx] = watch_loads(seen);
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::run_on_world_loading_hook("Boston", true);

    CHECK(seen->calls == 1);
    CHECK(seen->world == "Boston");
    CHECK(seen->reading_data);
}

// Rebuilding a world already in memory takes a moment rather than half a minute,
// so a handler that says something about the wait can tell the two apart.
TEST_CASE("lua_hook_on_world_loading_says_when_no_data_is_read", "[lua]") {
    const auto seen = std::make_shared<seen_load>();

    const auto [list, idx] = watch_loads(seen);
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::run_on_world_loading_hook("Boston", false);

    CHECK(seen->calls == 1);
    CHECK_FALSE(seen->reading_data);
}

// A world can be started before one has been picked, and an empty name is what
// that looks like -- the hook still has to fire, since the wait happens either
// way and a handler announcing it must not fall silent on that path.
TEST_CASE("lua_hook_on_world_loading_fires_without_a_named_world", "[lua]") {
    const auto seen = std::make_shared<seen_load>();

    const auto [list, idx] = watch_loads(seen);
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::run_on_world_loading_hook(std::string(), true);

    CHECK(seen->calls == 1);
    CHECK(seen->world.empty());
}
