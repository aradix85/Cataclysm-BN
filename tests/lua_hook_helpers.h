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

// Empties a hook list for the duration of a test and puts the original back.
//
// The accessibility layer is loaded into the world the suite builds, so its own
// handlers are registered in this state and "nothing is listening" is never true
// of it by default. Without this, a test asserting that an unregistered hook
// stays idle passes or fails according to what ran before it -- which is the
// cross-contamination upstream's issue #3146 is about, and it is invisible: the
// assertion still reads correct.
struct emptied_hook {
    sol::table hooks;
    std::string name;
    sol::table original;

    emptied_hook(sol::state& lua, std::string hook_name)
        : hooks(lua["game"]["hooks"].get<sol::table>()),
          name(std::move(hook_name)),
          original(hooks[name].get<sol::table>()) {
        hooks[name] = lua.create_table();
    }
    ~emptied_hook() { hooks[name] = original; }
};

} // namespace test_lua_hooks
