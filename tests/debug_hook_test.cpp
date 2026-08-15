#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "debug_hook.h"
#include "lua_hook_helpers.h"

#include <memory>
#include <string>

// The error report itself cannot be driven from here: `realDebugmsg` returns
// before showing anything while `test_mode` is set, and the screen it would
// otherwise raise needs a curses window the test binary never creates. So these
// assert the helper the one line in src/debug.cpp calls, which is where every
// decision about what the hook carries lives.

namespace {

struct seen_error {
    int calls = 0;
    std::string text;
};

} // namespace

// The report draws the function, the file, the line and the game version around
// the error. None of that is speech, and a handler that had to strip it would be
// parsing a layout that upstream is free to change, so the hook carries the error
// on its own.
TEST_CASE("lua_hook_on_debugmsg_carries_the_error_text", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_error>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_debugmsg", 0, [seen](sol::table params) {
            ++seen->calls;
            seen->text = params["text"].get<sol::optional<std::string>>().value_or("");
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::run_on_debugmsg_hook("Attempted to load unknown item id.");

    CHECK(seen->calls == 1);
    CHECK(seen->text == "Attempted to load unknown item id.");
}

// A handler that fails is reported by debugmsg, which raises the same screen
// again and would call the same failing handler a second time. The guard has to
// be released afterwards as well, or the first error of a session would be the
// last one ever spoken.
TEST_CASE("lua_hook_on_debugmsg_does_not_re_enter", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto calls = std::make_shared<int>(0);
    const auto running_inside = std::make_shared<bool>(false);

    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_debugmsg", 0, [calls, running_inside](sol::table) {
            ++*calls;
            *running_inside = cata::is_running_on_debugmsg_hook();
            // What a handler that trips a debugmsg of its own ends up doing.
            cata::run_on_debugmsg_hook("Reported while reporting.");
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::run_on_debugmsg_hook("The first one.");

    CHECK(*calls == 1);
    CHECK(*running_inside);
    CHECK_FALSE(cata::is_running_on_debugmsg_hook());

    // Released, so the next error reaches the handler again.
    cata::run_on_debugmsg_hook("A later one.");
    CHECK(*calls == 2);
}

// Over a thousand places in the source can raise a report, and any mod may be
// loaded without a handler for one. An unregistered hook must build no tables and
// must leave the guard alone.
TEST_CASE("lua_hook_on_debugmsg_is_a_no_op_when_unregistered", "[lua]") {
    REQUIRE_FALSE(cata::has_hooks("on_debugmsg"));

    cata::run_on_debugmsg_hook("Nobody is listening.");

    CHECK_FALSE(cata::is_running_on_debugmsg_hook());
}
