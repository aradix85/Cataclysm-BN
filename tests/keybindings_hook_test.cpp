#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "catch/catch.hpp"
#include "input.h"
#include "keybindings_hook.h"
#include "lua_hook_helpers.h"
#include "translations.h"

#include <memory>
#include <string>
#include <vector>

// The keybindings screen is the game's own answer to "which keys work here",
// and it is reached from 81 contexts, so what it hands over has to be right in
// all of them. The screen itself cannot be driven from a test -- it draws and
// blocks on input -- but the mapping it depends on can: which row a scroll
// offset points at, and what that row is called. Announcing the wrong row is
// silent to a player who cannot see the list, so that mapping is what these
// cases exist to break.

namespace
{

struct seen_screen {
    int calls = 0;
    std::string category;
    std::string title;
    int count = 0;
    sol::optional<int> cursor;
    bool has_entry = false;
    std::string entry_text;
    std::string entry_column;
};

void record( const std::shared_ptr<seen_screen> &out, const sol::table &params )
{
    ++out->calls;
    out->category = params["category"].get<sol::optional<std::string>>().value_or( "" );
    out->title = params["title"].get<sol::optional<std::string>>().value_or( "" );
    out->count = params["count"].get<sol::optional<int>>().value_or( -1 );
    out->cursor = params["cursor"].get<sol::optional<int>>();

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    if( !entry ) {
        return;
    }
    out->entry_text = ( *entry )["text"].get<std::string>();
    out->entry_column = ( *entry )["column"].get<std::string>();
}

// Names of this file's own, so that what the hook reports is fixed here rather
// than by whatever the game's data happens to call an action. Filled in place
// rather than returned, because a context's copy constructor exists only in the
// Android build.
void name_actions( input_context &ctxt )
{
    ctxt.register_action( "BN_TEST_ONE", to_translation( "Ask what is around you" ) );
    ctxt.register_action( "BN_TEST_TWO", to_translation( "Wield an item" ) );
    ctxt.register_action( "BN_TEST_THREE", to_translation( "Close the window" ) );
}

const std::vector<std::string> rows{ "BN_TEST_ONE", "BN_TEST_TWO", "BN_TEST_THREE" };

} // namespace

// The screen has to arrive whole: whose keys are being listed, what the screen
// is called, how many rows the filter left, and the row the view sits on with
// its name and its keys. That is the same shape a menu arrives in, which is
// what lets one reading model answer both.
TEST_CASE( "lua_hook_on_keybindings_carries_the_screen_and_the_row_in_view", "[lua]" )
{
    sol::state &lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::push_hook( lua, "on_keybindings", 0,
    [seen]( sol::table params ) {
        record( seen, params );
    } );
    test_lua_hooks::hook_cleanup cleanup{ list, idx };

    input_context ctxt( "BN_ACCESS_TEST" );
    name_actions( ctxt );
    cata::fire_on_keybindings( ctxt, "INVENTORY", rows, 0 );

    CHECK( seen->calls == 1 );
    // The context being described, not the one the screen itself runs in.
    CHECK( seen->category == "INVENTORY" );
    CHECK( seen->title == "Keybindings" );
    CHECK( seen->count == 3 );
    REQUIRE( seen->cursor.has_value() );
    CHECK( *seen->cursor == 1 );
    REQUIRE( seen->has_entry );
    CHECK( seen->entry_text == "Ask what is around you" );
    // The keys, in the game's own words -- which for an action nothing is bound
    // to is a statement that it is unbound, and never an empty string.
    CHECK( seen->entry_column == ctxt.get_desc( "BN_TEST_ONE" ) );
    CHECK_FALSE( seen->entry_column.empty() );
}

// The screen scrolls a window over the list instead of moving a cursor, so the
// offset is the only thing saying which row a keypress landed on. Off by one
// here means every scroll announces its neighbour.
TEST_CASE( "lua_hook_on_keybindings_reports_the_row_the_offset_points_at", "[lua]" )
{
    sol::state &lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::push_hook( lua, "on_keybindings", 0,
    [seen]( sol::table params ) {
        record( seen, params );
    } );
    test_lua_hooks::hook_cleanup cleanup{ list, idx };

    input_context ctxt( "BN_ACCESS_TEST" );
    name_actions( ctxt );

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, 1 );
    REQUIRE( seen->cursor.has_value() );
    CHECK( *seen->cursor == 2 );
    REQUIRE( seen->has_entry );
    CHECK( seen->entry_text == "Wield an item" );

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, 2 );
    REQUIRE( seen->cursor.has_value() );
    CHECK( *seen->cursor == 3 );
    CHECK( seen->entry_text == "Close the window" );
}

// Typing a filter that matches nothing leaves the screen open on an empty list,
// and the offset it kept from before then points at nothing. Both params stay
// unset rather than zeroed, so a handler reads "nothing to report" instead of
// naming a row that is not on screen.
TEST_CASE( "lua_hook_on_keybindings_has_no_row_when_the_filter_left_none", "[lua]" )
{
    sol::state &lua = test_lua_hooks::global_lua_state();

    const auto seen = std::make_shared<seen_screen>();
    const auto [list, idx] = test_lua_hooks::push_hook( lua, "on_keybindings", 0,
    [seen]( sol::table params ) {
        record( seen, params );
    } );
    test_lua_hooks::hook_cleanup cleanup{ list, idx };

    input_context ctxt( "BN_ACCESS_TEST" );
    name_actions( ctxt );

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", {}, 0 );
    CHECK( seen->calls == 1 );
    CHECK( seen->count == 0 );
    CHECK_FALSE( seen->cursor.has_value() );
    CHECK_FALSE( seen->has_entry );

    // An offset left pointing past the end of a shorter list answers the same.
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, 3 );
    CHECK( seen->calls == 2 );
    CHECK( seen->count == 3 );
    CHECK_FALSE( seen->cursor.has_value() );
    CHECK_FALSE( seen->has_entry );
}

// The hook has to exist as a table before anything can register into it: a name
// missing from `hook_names` makes every registration fail without a word. And
// since this fires on every keypress the screen sees, an unregistered hook must
// build no tables at all.
TEST_CASE( "lua_hook_on_keybindings_is_declared_and_idle_when_nothing_listens", "[lua]" )
{
    sol::state &lua = test_lua_hooks::global_lua_state();

    const auto declared = lua["game"]["hooks"]["on_keybindings"].get<sol::optional<sol::table>>();
    CHECK( declared.has_value() );

    const test_lua_hooks::emptied_hook empty{ lua, "on_keybindings" };
    REQUIRE_FALSE( cata::has_hooks( "on_keybindings" ) );

    input_context ctxt( "BN_ACCESS_TEST" );
    name_actions( ctxt );
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, 0 );
}
