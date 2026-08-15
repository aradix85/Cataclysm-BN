#include "catalua.h"
#include "catalua_hooks.h"
#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "mod_manager.h"

#include <memory>
#include <string>
#include <vector>

// The parse of a hook list is cached per hook name for the whole process, and
// invalidated by the raw length of that list. Two Lua states holding the same
// number of handlers for one name therefore have equal lengths, and the second
// one to fire would inherit the first one's parse: an entry read as a plain
// function used against a table, or one state's priority order imposed on the
// other's list.
//
// Nothing in the engine states that only one Lua state may exist at a time,
// which is the assumption the cache was written under.

namespace {

auto fresh_state() -> std::unique_ptr<cata::lua_state, cata::lua_state_deleter> {
    auto state = cata::make_wrapped_state();
    cata::init_global_state_tables(*state, std::vector<mod_id>{});
    cata::define_hooks(*state);
    return state;
}

// The two shapes a hook entry may take. One each, so both lists are one long
// and the length check cannot tell the states apart.
void register_table_form(cata::lua_state& state, const char* mark) {
    state.lua.script(
        std::string("game.hooks.on_dialogue_start[1] = { priority = 0, fn = function( params ) "
                    "params.results.mark = '")
        + mark + "' end }");
}

void register_function_form(cata::lua_state& state, const char* mark) {
    state.lua.script(
        std::string("game.hooks.on_dialogue_start[1] = function( params ) "
                    "params.results.mark = '")
        + mark + "' end");
}

auto fire(cata::lua_state& state) -> std::string {
    sol::table results = cata::run_hooks("on_dialogue_start", nullptr, {.state = &state});
    return results.get_or<std::string>("mark", "nothing ran");
}

} // namespace

TEST_CASE("hook_entries_are_not_shared_between_states", "[lua]") {
    SECTION("table form parsed first") {
        auto first = fresh_state();
        auto second = fresh_state();
        register_table_form(*first, "first");
        register_function_form(*second, "second");

        CHECK(fire(*first) == "first");
        CHECK(fire(*second) == "second");
    }

    SECTION("function form parsed first") {
        auto first = fresh_state();
        auto second = fresh_state();
        register_function_form(*first, "first");
        register_table_form(*second, "second");

        CHECK(fire(*first) == "first");
        CHECK(fire(*second) == "second");
    }

    SECTION("a state keeps answering after another has fired") {
        auto first = fresh_state();
        auto second = fresh_state();
        register_table_form(*first, "first");
        register_function_form(*second, "second");

        CHECK(fire(*first) == "first");
        CHECK(fire(*second) == "second");
        CHECK(fire(*first) == "first");
    }
}
