#pragma once

#include "catalua_impl.h"
#include "catalua_sol.h"
#include "init.h"

#include <string>
#include <utility>

// Scaffolding for tests that need a hook backed by a C++ callable rather than a
// Lua script, so the assertions can live next to the C++ they are about.

namespace test_lua_hooks {

inline sol::state& global_lua_state() { return DynamicDataLoader::get_instance().lua->lua; }

// Append a hook entry to a global hook list. Returns the list and the index so
// the caller can remove it again.
template <typename Fn>
auto push_hook(sol::state& lua, const std::string& name, int priority, Fn&& fn)
    -> std::pair<sol::table, int> {
    sol::table list = lua["game"]["hooks"][name];
    auto* L = lua.lua_state();
    sol::stack::push(L, list);
    const auto idx = static_cast<int>(lua_rawlen(L, -1)) + 1;
    lua_pop(L, 1);

    sol::table entry = lua.create_table();
    entry["mod_id"] = "test";
    entry["priority"] = priority;
    entry["fn"] = std::forward<Fn>(fn);
    list[idx] = entry;
    return {list, idx};
}

// Removes the entry again even when an assertion throws and unwinds the stack,
// so one test cannot leave a hook registered for the next.
struct hook_cleanup {
    sol::table list;
    int idx;
    hook_cleanup(sol::table l, int i): list(std::move(l)), idx(i) {}
    ~hook_cleanup() { list[idx] = sol::lua_nil; }
};

} // namespace test_lua_hooks
