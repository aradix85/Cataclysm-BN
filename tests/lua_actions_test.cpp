#include "action.h"
#include "catalua_hooks.h"
#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "debug.h"
#include "input.h"
#include "lua_actions.h"
#include "lua_hook_helpers.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// Tests for command dispatch: the registry of mod-owned actions, and the hook
// that hands a keypress to Lua before the game resolves it. Both run headless.
// What no test here can cover is that a physical key reaches the game -- that
// needs a real keypress and is judged in play.

namespace {

// input_context::is_action_registered is compiled only on Android, so the
// portable check goes through the copying accessor.
bool registered_in(const input_context& ctxt, const std::string& action) {
    const std::vector<std::string> actions = ctxt.get_registered_actions_copy();
    return std::ranges::find(actions, action) != actions.end();
}

// The registry is global state, so every test starts from an empty one.
struct scoped_registry {
    scoped_registry() { cata::lua_actions::clear_actions(); }
    ~scoped_registry() { cata::lua_actions::clear_actions(); }
};

} // namespace

// The keybindings screen shows the display name, so it must survive
// registration. An omitted name falls back to the id rather than to an empty
// line, which would be an unreadable entry in that screen.
TEST_CASE("lua_action_registry_keeps_id_and_display_name", "[lua][actions]") {
    scoped_registry guard;

    cata::lua_actions::register_action("bn_access_status", "Status readout");
    cata::lua_actions::register_action("bn_access_surroundings", "");

    const auto& actions = cata::lua_actions::get_actions();
    REQUIRE(actions.size() == 2);
    CHECK(actions[0].id == "bn_access_status");
    CHECK(actions[0].name == "Status readout");
    CHECK(actions[1].id == "bn_access_surroundings");
    CHECK(actions[1].name == "bn_access_surroundings");
}

// Reloading a mod's scripts registers everything a second time. That must
// update the entry rather than add a duplicate, or the keybindings screen fills
// up with copies and one key ends up bound to several identical actions.
TEST_CASE("lua_action_registry_replaces_rather_than_duplicates", "[lua][actions]") {
    scoped_registry guard;

    cata::lua_actions::register_action("bn_access_status", "Status readout");
    cata::lua_actions::register_action("bn_access_status", "How am I doing");

    const auto& actions = cata::lua_actions::get_actions();
    REQUIRE(actions.size() == 1);
    CHECK(actions[0].name == "How am I doing");
}

// An id the game already uses would be resolved by look_up_action into a real
// action_id, so the mod's key would silently run the built-in command instead.
// That must be refused loudly at registration rather than discovered in play.
TEST_CASE("lua_action_registry_refuses_a_built_in_action_id", "[lua][actions]") {
    scoped_registry guard;
    REQUIRE(look_up_action("pause") != ACTION_NULL);

    const std::string msg = capture_debugmsg_during([]() {
        cata::lua_actions::register_action("pause", "Not mine to take");
    });

    CHECK(msg.find("pause") != std::string::npos);
    CHECK(cata::lua_actions::get_actions().empty());
}

// An empty id would register an action nothing can ever be bound to, and would
// sit in the keybindings screen as a nameless row.
TEST_CASE("lua_action_registry_refuses_an_empty_id", "[lua][actions]") {
    scoped_registry guard;

    const std::string msg = capture_debugmsg_during([]() {
        cata::lua_actions::register_action("", "Nameless");
    });

    CHECK_FALSE(msg.empty());
    CHECK(cata::lua_actions::get_actions().empty());
}

// The whole point of the registry: a context built after registration knows the
// action, which is what lets input_to_action ever return it, and carries the
// display name, which is what the keybindings screen lists it under.
TEST_CASE("lua_action_registry_registers_into_a_context", "[lua][actions]") {
    scoped_registry guard;

    cata::lua_actions::register_action("bn_access_status", "Status readout");
    input_context ctxt("DEFAULTMODE");
    REQUIRE_FALSE(registered_in(ctxt, "bn_access_status"));

    cata::lua_actions::register_all(ctxt);

    CHECK(registered_in(ctxt, "bn_access_status"));
    CHECK(ctxt.get_action_name("bn_access_status") == "Status readout");
}

// A world loaded without the mod must not inherit its actions, which is what
// clearing on Lua state teardown is for.
TEST_CASE("lua_action_registry_clears", "[lua][actions]") {
    scoped_registry guard;
    cata::lua_actions::register_action("bn_access_status", "Status readout");
    REQUIRE(cata::lua_actions::get_actions().size() == 1);

    cata::lua_actions::clear_actions();

    CHECK(cata::lua_actions::get_actions().empty());
    input_context ctxt("DEFAULTMODE");
    cata::lua_actions::register_all(ctxt);
    CHECK_FALSE(registered_in(ctxt, "bn_access_status"));
}

// The hook has to carry which action was pressed, or a handler cannot tell one
// of its own commands from another.
TEST_CASE("lua_hook_on_action_carries_the_action_string", "[lua][actions]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<std::string>();
    const auto calls = std::make_shared<int>(0);

    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_action", 0, [seen, calls](sol::table params) {
            ++*calls;
            *seen = params["action"].get<sol::optional<std::string>>().value_or("");
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    const bool claimed = cata::lua_actions::run_on_action_hook("bn_access_status");

    CHECK(*calls == 1);
    CHECK(*seen == "bn_access_status");
    // Observing an action is not claiming it: the game still resolves it.
    CHECK_FALSE(claimed);
}

// Returning false claims the action. The caller in handle_action.cpp then
// returns before look_up_action, so an unknown action string never reaches the
// action switch and no turn passes. A claim also stops lower-priority hooks,
// so two mods cannot both act on one keypress.
TEST_CASE("lua_hook_on_action_claim_stops_the_game_and_later_hooks", "[lua][actions]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto lower_ran = std::make_shared<bool>(false);

    const auto [list_hi, idx_hi] =
        test_lua_hooks::push_hook(lua, "on_action", 10, [](sol::table) -> bool { return false; });
    test_lua_hooks::hook_cleanup cleanup_hi{list_hi, idx_hi};

    const auto [list_lo, idx_lo] = test_lua_hooks::
        push_hook(lua, "on_action", 5, [lower_ran](sol::table) { *lower_ran = true; });
    test_lua_hooks::hook_cleanup cleanup_lo{list_lo, idx_lo};

    CHECK(cata::lua_actions::run_on_action_hook("bn_access_status"));
    CHECK_FALSE(*lower_ran);
}

// With nothing registered the hook must build no tables and claim nothing, so
// every keypress in a game without the mod costs the same as before.
TEST_CASE("lua_hook_on_action_is_a_no_op_when_unregistered", "[lua][actions]") {
    const test_lua_hooks::emptied_hook empty{test_lua_hooks::global_lua_state(), "on_action"};
    REQUIRE_FALSE(cata::has_hooks("on_action"));

    CHECK_FALSE(cata::lua_actions::run_on_action_hook("bn_access_status"));
    CHECK_FALSE(cata::lua_actions::run_on_action_hook(""));
}

// The mod's main.lua calls these bindings by name from Lua, where a typo or an
// arity change surfaces only as a debugmsg in a running game -- something the
// player this project is for cannot read. So the Lua-visible surface is pinned
// here instead.
TEST_CASE("lua_input_context_bindings", "[lua][actions]") {
    scoped_registry guard;

    sol::state lua = make_lua_state();
    sol::table test_data = lua.create_table();
    lua.globals()["test_data"] = test_data;

    run_lua_script(lua, "tests/lua/input_context_bindings_test.lua");

    const sol::table out = test_data["out"];
    CHECK(out["known"].get<bool>());
    CHECK(out["named"].get<bool>());
    // register_directions registers the movement actions as a group.
    CHECK(out["directions"].get<bool>());
    CHECK_FALSE(out["unknown"].get<bool>());

    // The registry call went through the binding, including the arity where the
    // display name is left out.
    const auto& actions = cata::lua_actions::get_actions();
    REQUIRE(actions.size() == 2);
    CHECK(actions[0].name == "Test action");
    CHECK(actions[1].name == "bn_access_test_action_unnamed");
}

// The input layer answers with sentinels when nothing was pressed or when a key
// matched nothing. The activity poll runs ten times a second, so firing the hook
// on those would call into Lua continuously while the character is busy.
TEST_CASE("lua_hook_on_action_ignores_input_sentinels", "[lua][actions]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto calls = std::make_shared<int>(0);
    const auto [list, idx] = test_lua_hooks::push_hook(lua, "on_action", 0, [calls](sol::table) {
        ++*calls;
    });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    CHECK_FALSE(cata::lua_actions::run_on_action_hook("TIMEOUT"));
    CHECK_FALSE(cata::lua_actions::run_on_action_hook("ERROR"));
    CHECK_FALSE(cata::lua_actions::run_on_action_hook("ANY_INPUT"));
    CHECK_FALSE(cata::lua_actions::run_on_action_hook(""));
    CHECK(*calls == 0);

    // A real action still reaches the handler.
    cata::lua_actions::run_on_action_hook("bn_access_status");
    CHECK(*calls == 1);
}
