#include "catalua.h"
#include "catalua_hooks.h"
#include "catalua_impl.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "enums.h"
#include "init.h"
#include "messages_hook.h"

#include <memory>
#include <string>

// The call site of this hook lives in src/messages.cpp, which is replaced by
// tests/fake_messages.cpp in the test build, so these tests exercise the helper
// that messages.cpp calls rather than add_msg itself.

namespace {

// Append a hook entry backed by a C++ callable to a global hook list.
// Returns the list and the index so the caller can remove it again.
template <typename Fn>
auto push_hook(sol::state& lua, const std::string& name, Fn&& fn) -> std::pair<sol::table, int> {
    sol::table list = lua["game"]["hooks"][name];
    auto* L = lua.lua_state();
    sol::stack::push(L, list);
    const auto idx = static_cast<int>(lua_rawlen(L, -1)) + 1;
    lua_pop(L, 1);

    auto entry = lua.create_table();
    entry["mod_id"] = "test";
    entry["priority"] = 0;
    entry["fn"] = std::forward<Fn>(fn);
    list[idx] = entry;
    return {list, idx};
}

// Removes the entry again even when an assertion throws and unwinds the stack.
struct hook_cleanup {
    sol::table list;
    int idx;
    hook_cleanup(sol::table l, int i): list(l), idx(i) {}
    ~hook_cleanup() { list[idx] = sol::lua_nil; }
};

sol::state& global_lua_state() { return DynamicDataLoader::get_instance().lua->lua; }

} // namespace

// Pins down that the hook carries the message text and its type, which is what
// lets a handler filter without re-parsing the message.
TEST_CASE("lua_hook_on_add_msg_carries_text_and_type", "[lua]") {
    sol::state& lua = global_lua_state();

    const auto seen_text = std::make_shared<std::string>();
    const auto seen_type = std::make_shared<game_message_type>(m_neutral);
    const auto calls = std::make_shared<int>(0);

    const auto [list, idx] =
        push_hook(lua, "on_add_msg", [seen_text, seen_type, calls](sol::table params) {
            ++*calls;
            *seen_text = params["text"].get<sol::optional<std::string>>().value_or("");
            *seen_type = params["type"].get<sol::optional<game_message_type>>().value_or(m_neutral);
        });
    hook_cleanup cleanup{list, idx};

    cata::run_on_add_msg_hook("You feel weak.", m_bad);

    CHECK(*calls == 1);
    CHECK(*seen_text == "You feel weak.");
    CHECK(*seen_type == m_bad);
}

// A handler that adds a message of its own must not re-enter the hook: without
// the guard this recurses until the stack runs out. The nested call is a no-op
// and the guard must be released again afterwards, or every later message would
// be silently dropped.
TEST_CASE("lua_hook_on_add_msg_does_not_re_enter", "[lua]") {
    sol::state& lua = global_lua_state();

    const auto calls = std::make_shared<int>(0);
    const auto running_inside = std::make_shared<bool>(false);

    const auto [list, idx] = push_hook(lua, "on_add_msg", [calls, running_inside](sol::table) {
        ++*calls;
        *running_inside = cata::is_running_on_add_msg_hook();
        // What a handler that emits its own message would end up doing.
        cata::run_on_add_msg_hook("Nested message.", m_neutral);
    });
    hook_cleanup cleanup{list, idx};

    cata::run_on_add_msg_hook("Outer message.", m_neutral);

    CHECK(*calls == 1);
    CHECK(*running_inside);
    CHECK_FALSE(cata::is_running_on_add_msg_hook());

    // The guard is released, so the next message reaches the handler again.
    cata::run_on_add_msg_hook("Later message.", m_neutral);
    CHECK(*calls == 2);
}

// With nothing registered the hook must build no tables and must leave the guard
// alone, so an unused hook stays free.
TEST_CASE("lua_hook_on_add_msg_is_a_no_op_when_unregistered", "[lua]") {
    REQUIRE_FALSE(cata::has_hooks("on_add_msg"));

    cata::run_on_add_msg_hook("Nobody is listening.", m_info);

    CHECK_FALSE(cata::is_running_on_add_msg_hook());
}
