#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "filesystem.h"

#include <algorithm>
#include <string>
#include <vector>

// One case that runs every Lua test script the fork owns.
//
// The layer itself is Lua, and Lua compiles nothing: a change to it is playable
// the moment the file is saved. Its assertions used to be C++ regardless, so
// every new case cost a link of the test binary, which is the long part of a
// build here. This harness moves that cost to one file: a script dropped into
// tests/lua/bn_access/ is picked up by name, with nothing to declare and
// nothing to compile.
//
// Scripts assert through the `check` table, which is loaded here and handed to
// each of them as a global -- see tests/lua/bn_access/check.lua.
//
// Each script gets a state of its own, so one cannot leave a module or a global
// behind for the next, and the order they run in cannot change an outcome.

namespace {

// Run from the clone root, the way every other test that reads a data file is:
// the binary resolves these against the working directory.
const std::string script_dir = "tests/lua/bn_access";

} // namespace

TEST_CASE("bn_access_lua_scripts", "[lua]") {
    std::vector<std::string> scripts = get_files_from_path("_test.lua", script_dir, false, true);
    std::ranges::sort(scripts);

    // An empty folder would otherwise be a green run proving nothing at all,
    // and this case is the only thing standing behind every script in it.
    REQUIRE(!scripts.empty());

    for (const std::string& script : scripts) {
        INFO(script);

        sol::state lua = make_lua_state();

        sol::load_result loaded = lua.load_file(script_dir + "/check.lua");
        REQUIRE(loaded.valid());
        sol::protected_function_result made = sol::protected_function(loaded)();
        REQUIRE(made.valid());
        sol::table check = made;
        lua.globals()["check"] = check;

        // A script that fails to parse or raises an error reports as itself
        // rather than ending the whole case, so one broken script cannot hide
        // the results of the others.
        try {
            run_lua_script(lua, script);
        } catch (const std::exception& err) {
            INFO(err.what());
            FAIL_CHECK("script did not run to the end");
            continue;
        }

        // A script that asserts nothing passes exactly like one that asserts
        // everything, so silence is a failure here.
        CHECK(check["count"].get<int>() > 0);

        std::string report;
        sol::table failures = check["failures"];
        for (size_t i = 1; i <= failures.size(); i++) {
            report += "\n  " + failures[i].get<std::string>();
        }
        INFO(report);
        CHECK(report.empty());
    }
}
