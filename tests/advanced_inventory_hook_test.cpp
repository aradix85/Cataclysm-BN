#include "advanced_inventory_hook.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "lua_hook_helpers.h"

#include <memory>
#include <string>
#include <vector>

// A pane needs a player, a map and a curses window, so these drive the firing
// function with the rows and places the screen reads out of one. What that
// costs is the single call line in src/advanced_inv.cpp; what it buys is that
// every decision about what the player is told is asserted here rather than
// only at a keyboard, on a screen whose whole subject -- which pane, which
// square, from where to where -- is drawn and never written.

namespace {

struct seen_advanced {
    int calls = 0;
    std::string source_area;
    std::string source_vehicle;
    std::string destination_area;
    std::string destination_vehicle;
    int count = 0;
    sol::optional<int> cursor;
    bool has_entry = false;
    std::string entry_text;
    std::string entry_category;
    std::string entry_square;
};

// Reads one firing of the hook out of the params table.
void record(const std::shared_ptr<seen_advanced>& out, const sol::table& params) {
    ++out->calls;
    const auto source = params["source"].get<sol::table>();
    const auto destination = params["destination"].get<sol::table>();
    out->source_area = source["area"].get<std::string>();
    out->source_vehicle = source["vehicle"].get<std::string>();
    out->destination_area = destination["area"].get<std::string>();
    out->destination_vehicle = destination["vehicle"].get<std::string>();
    out->count = params["count"].get<sol::optional<int>>().value_or(-1);
    out->cursor = params["cursor"].get<sol::optional<int>>();

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    if (!entry) { return; }
    out->entry_text = (*entry)["text"].get<std::string>();
    out->entry_category = (*entry)["category"].get<std::string>();
    out->entry_square = (*entry)["square"].get<std::string>();
}

cata::advanced_inventory_place place(const std::string& area, const std::string& vehicle = "") {
    cata::advanced_inventory_place out;
    out.area = area;
    out.vehicle = vehicle;
    return out;
}

cata::advanced_inventory_line row(const std::string& text) {
    cata::advanced_inventory_line line;
    line.text = text;
    return line;
}

// A pocket knife, a rock and two cans of beans, with the cursor on the rock.
std::vector<cata::advanced_inventory_line> pile() {
    std::vector<cata::advanced_inventory_line> lines{
        row("pocket knife"),
        row("rock"),
        row("2 cans of beans"),
    };
    lines[1].selected = true;
    return lines;
}

} // namespace

// The pair of places is the screen. Items leave the pane the cursor is in and
// arrive in the other one, and which is which is drawn as a colour, so a firing
// that carried only the list would describe a screen the player cannot use.
TEST_CASE("lua_hook_on_advanced_inventory_carries_both_places", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_advanced>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_advanced_inventory", 0, [seen](sol::table params) {
            record(seen, params);
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_advanced_inventory(place("Inventory"), place("South"), pile());

    CHECK(seen->calls == 1);
    CHECK(seen->source_area == "Inventory");
    CHECK(seen->destination_area == "South");
    CHECK(seen->count == 3);
    REQUIRE(seen->cursor.has_value());
    CHECK(*seen->cursor == 2);
    REQUIRE(seen->has_entry);
    CHECK(seen->entry_text == "rock");
}

// A square with a vehicle on it holds two separate places, and the key that
// swaps between them changes nothing else that can be heard. The direction
// travels with the vehicle's name because it is the half that says where the
// player has to stand.
TEST_CASE("lua_hook_on_advanced_inventory_names_the_vehicle_and_keeps_the_direction", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_advanced>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_advanced_inventory", 0, [seen](sol::table params) {
            record(seen, params);
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_advanced_inventory(place("South", "shopping cart"), place("Inventory"), pile());

    CHECK(seen->source_area == "South");
    CHECK(seen->source_vehicle == "shopping cart");
    CHECK(seen->destination_vehicle.empty());
}

// The heading and the square are the structure the screen draws and never
// writes: a heading only while the pane is sorted by category, a square only
// while it is showing everything around the player at once.
TEST_CASE("lua_hook_on_advanced_inventory_carries_the_heading_and_the_square", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_advanced>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_advanced_inventory", 0, [seen](sol::table params) {
            record(seen, params);
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    std::vector<cata::advanced_inventory_line> lines = pile();
    lines[1].category = "TOOLS";
    lines[1].square = "South West";
    cata::fire_on_advanced_inventory(place("Surrounding area"), place("Inventory"), lines);

    REQUIRE(seen->has_entry);
    CHECK(seen->entry_category == "TOOLS");
    CHECK(seen->entry_square == "South West");
}

// An empty pane, or a filter that matched nothing, leaves no selection at all.
// Handing over a zeroed one would have the layer name a row that is not there,
// which is the failure that cannot be noticed without sight.
TEST_CASE("lua_hook_on_advanced_inventory_leaves_the_selection_unset_when_there_is_none", "[lua]") {
    sol::state& lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_advanced>();
    const auto [list, idx] =
        test_lua_hooks::push_hook(lua, "on_advanced_inventory", 0, [seen](sol::table params) {
            record(seen, params);
        });
    test_lua_hooks::hook_cleanup cleanup{list, idx};

    cata::fire_on_advanced_inventory(place("Inventory"), place("South"), {});

    CHECK(seen->calls == 1);
    CHECK(seen->count == 0);
    CHECK_FALSE(seen->cursor.has_value());
    CHECK_FALSE(seen->has_entry);
}
