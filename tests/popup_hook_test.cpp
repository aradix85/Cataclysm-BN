#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"
#include "popup.h"
#include "popup_hook.h"

#include <memory>
#include <string>
#include <vector>

// These drive a real query_popup through `query_once`, which fires the hook
// before it touches any window, so the single call line in src/popup.cpp is
// covered here rather than only the helper behind it.

namespace {

struct seen_popup {
    int calls = 0;
    std::string text;
    std::string category;
    std::vector<std::string> ids;
    std::vector<std::string> names;
    sol::optional<int> cursor;
};

// Reads one firing of the hook out of the params table.
void record(const std::shared_ptr<seen_popup>& out, const sol::table& params) {
    ++out->calls;
    out->text = params["text"].get<sol::optional<std::string>>().value_or("");
    out->category = params["category"].get<sol::optional<std::string>>().value_or("");
    out->cursor = params["cursor"].get<sol::optional<int>>();

    out->ids.clear();
    out->names.clear();
    const sol::table options = params["options"];
    const int n = static_cast<int>(options.size());
    for (int i = 1; i <= n; ++i) {
        const sol::table option = options[i];
        out->ids.push_back(option["id"].get<std::string>());
        out->names.push_back(option["name"].get<std::string>());
    }
}

} // namespace

// A prompt seizes the keyboard and says nothing on its own, so everything needed
// to speak it has to arrive in one firing: the question, and each option with the
// same name the popup puts on its own button. An id such as "YES" is not speakable
// text and is not translated, so resolving the name here is the point of the hook.
TEST_CASE("lua_hook_on_query_popup_carries_the_question_and_its_options", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_popup>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_query_popup", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    query_popup()
        .context("YESNO")
        .message("%s", "You may be attacked!  Proceed?")
        .option("YES")
        .option("NO")
        .query_once();

    CHECK(seen->calls == 1);
    CHECK(seen->text == "You may be attacked!  Proceed?");
    CHECK(seen->category == "YESNO");
    CHECK(seen->ids == std::vector<std::string>{"YES", "NO"});
    CHECK(seen->names == std::vector<std::string>{"Yes", "No"});
}

// Which option is selected must arrive with the prompt. Not announcing the
// selection is F2, the failure a blind CDDA player described as costing
// playability outright, and the hook is where that information enters Lua.
// The index is 1-based, so it can index `options` directly in Lua.
TEST_CASE("lua_hook_on_query_popup_reports_the_selected_option", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_popup>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_query_popup", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    query_popup()
        .context("YESNOQUIT")
        .message("%s", "Save before quitting?")
        .option("YES")
        .option("NO")
        .option("QUIT")
        .cursor(2)
        .query_once();

    REQUIRE(seen->calls == 1);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 3);
    CHECK(seen->ids[*seen->cursor - 1] == "QUIT");
}

// A popup that just waits for any key has nothing to select. Leaving `cursor`
// unset rather than zero stops a handler from announcing a selection that does
// not exist, and lets it tell the two kinds of prompt apart.
TEST_CASE("lua_hook_on_query_popup_has_no_cursor_without_options", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_popup>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_query_popup", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    query_popup()
        .context("POPUP_WAIT")
        .message("%s", "Please wait.")
        .allow_anykey(true)
        .query_once();

    REQUIRE(seen->calls == 1);
    CHECK(seen->ids.empty());
    CHECK_FALSE(seen->cursor.has_value());
}

// A handler that fails trips a debugmsg, and a debugmsg raises a popup of its
// own. Without the guard that recurses until the stack runs out, taking the game
// down at the exact moment the player is being asked something. The guard must be
// released again afterwards, or every later prompt would be silently unspoken.
TEST_CASE("lua_hook_on_query_popup_does_not_re_enter", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto calls = std::make_shared<int>(0);
    const auto running_inside = std::make_shared<bool>(false);

    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_query_popup", 0, [calls, running_inside](sol::table) {
            ++*calls;
            *running_inside = cata::is_running_on_query_popup_hook();
            // What a handler that trips a debugmsg would end up doing.
            query_popup().context("YESNO").message("%s", "Nested.").option("YES").query_once();
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    query_popup().context("YESNO").message("%s", "Outer.").option("YES").query_once();

    CHECK(*calls == 1);
    CHECK(*running_inside);
    CHECK_FALSE(cata::is_running_on_query_popup_hook());

    // The guard is released, so the next prompt reaches the handler again.
    query_popup().context("YESNO").message("%s", "Later.").option("YES").query_once();
    CHECK(*calls == 2);
}

// Every popup in the game fires this, including the ones on the main menu, so an
// unused hook must build no tables and must leave the guard alone.
TEST_CASE("lua_hook_on_query_popup_is_a_no_op_when_unregistered", "[lua]") {
    REQUIRE_FALSE(cata::has_hooks("on_query_popup"));

    cata::run_on_query_popup_hook("YESNO", "Nobody is listening.", {"YES", "NO"}, 0);

    CHECK_FALSE(cata::is_running_on_query_popup_hook());
}
