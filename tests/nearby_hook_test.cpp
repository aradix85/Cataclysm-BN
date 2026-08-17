#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "lua_hook_helpers.h"
#include "nearby_hook.h"

#include <memory>
#include <string>
#include <vector>

// The list of nearby items exists to answer where something is, and the screen
// prints no direction at all -- it draws a trail across the map instead. So what is
// asserted here is that a row's square arrives as an offset, and that the selection
// is the row the offset belongs to.

namespace {

struct seen_list {
    int calls = 0;
    int count = -1;
    sol::optional<int> cursor;
    bool has_entry = false;
    std::string text;
    std::string category;
    int stack = 0;
    int dx = 0;
    int dy = 0;
    int dz = 0;
};

void record(const std::shared_ptr<seen_list>& out, const sol::table& params) {
    ++out->calls;
    out->count = params["count"].get<sol::optional<int>>().value_or(-1);
    out->cursor = params["cursor"].get<sol::optional<int>>();

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    if (!entry) { return; }
    out->text = (*entry)["text"].get<std::string>();
    out->category = (*entry)["category"].get<std::string>();
    out->stack = (*entry)["count"].get<int>();
    out->dx = (*entry)["dx"].get<int>();
    out->dy = (*entry)["dy"].get<int>();
    out->dz = (*entry)["dz"].get<int>();
}

std::vector<cata::nearby_row> three_rows() {
    std::vector<cata::nearby_row> rows;

    cata::nearby_row rock;
    rock.text = "rock";
    rock.category = "MATERIALS";
    rock.offset = tripoint_rel_ms(0, -4, 0);
    rows.push_back(rock);

    cata::nearby_row bandages;
    bandages.text = "2 bandages";
    bandages.category = "DRUGS";
    bandages.count = 2;
    bandages.offset = tripoint_rel_ms(3, 3, -1);
    bandages.selected = true;
    rows.push_back(bandages);

    cata::nearby_row hammer;
    hammer.text = "hammer";
    hammer.offset = tripoint_rel_ms(1, 0, 0);
    rows.push_back(hammer);

    return rows;
}

} // namespace

// The selected row arrives whole, and its square as an offset from the player. A
// row without its offset is a name with no way to reach it, which is what the
// screen leaves a player without sight holding.
TEST_CASE("lua_hook_on_nearby_items_carries_the_selected_row_and_its_square", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_list>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_nearby_items", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_nearby_items(three_rows(), "DOWN");

    CHECK(seen->calls == 1);
    CHECK(seen->count == 3);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 2);
    REQUIRE(seen->has_entry);
    CHECK(seen->text == "2 bandages");
    CHECK(seen->category == "DRUGS");
    CHECK(seen->stack == 2);
    CHECK(seen->dx == 3);
    CHECK(seen->dy == 3);
    CHECK(seen->dz == -1);
}

// A list nothing is selected in still has to arrive, because the screen stays open
// on it: it prints that it sees no items, and a handler that was told nothing at
// all would answer that keypress with silence.
TEST_CASE("lua_hook_on_nearby_items_has_no_cursor_when_nothing_is_selected", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_list>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_nearby_items", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_nearby_items({}, "");

    REQUIRE(seen->calls == 1);
    CHECK(seen->count == 0);
    CHECK_FALSE(seen->cursor.has_value());
    CHECK_FALSE(seen->has_entry);
}

// The screen answers its own keys and swallows the rest, and it reads them in a
// loop of its own. The sentinels are what that loop answers when nothing was
// pressed at all.
TEST_CASE("lua_hook_on_nearby_items_ignores_the_input_layer_sentinels", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_list>();
    const auto [list, idx] = test_lua_hooks::
        push_hook(lua, "on_nearby_items", 0, [seen](sol::table params) { record(seen, params); });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    for (const std::string& sentinel :
         {std::string("TIMEOUT"), std::string("ERROR"), std::string("ANY_INPUT")}) {
        cata::fire_on_nearby_items(three_rows(), sentinel);
    }
    CHECK(seen->calls == 0);

    cata::fire_on_nearby_items(three_rows(), "");
    CHECK(seen->calls == 1);
}

// The hook has to exist as a table before any mod can insert into it: a name
// missing from `hook_names` makes every registration fail without a word.
TEST_CASE("lua_hook_on_nearby_items_is_declared_and_idle_when_nothing_listens", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_nearby_items"].get<sol::optional<sol::table>>();
    CHECK(declared.has_value());

    const test_lua_hooks::emptied_hook empty{lua, "on_nearby_items"};
    REQUIRE_FALSE(cata::has_hooks("on_nearby_items"));

    cata::fire_on_nearby_items(three_rows(), "DOWN");
}
