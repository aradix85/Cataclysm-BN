#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "direction_hook.h"
#include "lua_hook_helpers.h"

#include <memory>
#include <string>

// The question a verb asks when it needs a square to act on. Its own popup, its own
// input loop, and no way in for anything else in the layer: what this firing point
// hands over is all that prompt will ever say. So what is asserted here is that the
// question arrives in the game's own words, that whether up and down answer it
// arrives with it, and that a round the input layer answered with a sentinel is not
// a round at all -- firing on those would say the question again for a key the
// prompt never saw.

namespace {

struct seen_prompt {
    int calls = 0;
    std::string text;
    bool vertical = false;
    std::string action;
};

void record(const std::shared_ptr<seen_prompt>& out, const sol::table& params) {
    ++out->calls;
    out->text = params["text"].get<sol::optional<std::string>>().value_or("");
    out->vertical = params["vertical"].get<sol::optional<bool>>().value_or(false);
    out->action = params["action"].get<sol::optional<std::string>>().value_or("");
}

} // namespace

TEST_CASE("lua_hook_on_direction_prompt_carries_the_question", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_prompt>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_direction_prompt", 0, [seen](sol::table params) {
            record(seen, params);
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    // An empty action is the prompt opening, which is the one firing that has to
    // speak in full.
    cata::fire_on_direction_prompt("Close where?", false, "");

    REQUIRE(seen->calls == 1);
    CHECK(seen->text == "Close where?");
    CHECK_FALSE(seen->vertical);
    CHECK(seen->action.empty());

    // A key that did not answer it: the prompt is still standing, and the layer is
    // told which key was refused so it can tell the two firings apart.
    cata::fire_on_direction_prompt("Jump across where?", true, "HELP_KEYBINDINGS");

    REQUIRE(seen->calls == 2);
    CHECK(seen->text == "Jump across where?");
    CHECK(seen->vertical);
    CHECK(seen->action == "HELP_KEYBINDINGS");
}

// The loop this fires in has no timeout of its own, but the input layer still
// answers with these when nothing matched, and each one would re-say a question
// nobody was asked. Asserted rather than assumed, because the cost of getting it
// wrong is the prompt repeating itself at the speed of the input layer.
TEST_CASE("lua_hook_on_direction_prompt_drops_the_sentinels", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_prompt>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_direction_prompt", 0, [seen](sol::table params) {
            record(seen, params);
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_direction_prompt("Smash where?", false, "TIMEOUT");
    cata::fire_on_direction_prompt("Smash where?", false, "ERROR");
    cata::fire_on_direction_prompt("Smash where?", false, "ANY_INPUT");

    CHECK(seen->calls == 0);

    cata::fire_on_direction_prompt("Smash where?", false, "");

    CHECK(seen->calls == 1);
}
