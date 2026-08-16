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
    bool picking = false;
    std::string adds_local;
    std::string adds_global;
    std::string removes;
    sol::optional<int> cursor;
    bool has_entry = false;
    std::string entry_text;
    std::string entry_column;
    sol::optional<std::string> entry_letter;
    sol::optional<std::string> entry_scope;
};

void record( const std::shared_ptr<seen_screen> &out, const sol::table &params )
{
    ++out->calls;
    out->category = params["category"].get<sol::optional<std::string>>().value_or( "" );
    out->title = params["title"].get<sol::optional<std::string>>().value_or( "" );
    out->count = params["count"].get<sol::optional<int>>().value_or( -1 );
    out->picking = params["picking"].get<sol::optional<bool>>().value_or( false );
    out->cursor = params["cursor"].get<sol::optional<int>>();

    const auto keys = params["keys"].get<sol::optional<sol::table>>();
    if( keys ) {
        out->adds_local = ( *keys )["add_local"].get<std::string>();
        out->adds_global = ( *keys )["add_global"].get<std::string>();
        out->removes = ( *keys )["remove"].get<std::string>();
    }

    const auto entry = params["entry"].get<sol::optional<sol::table>>();
    out->has_entry = entry.has_value();
    out->entry_letter = sol::optional<std::string>();
    out->entry_scope = sol::optional<std::string>();
    if( !entry ) {
        return;
    }
    out->entry_text = ( *entry )["text"].get<std::string>();
    out->entry_column = ( *entry )["column"].get<std::string>();
    out->entry_letter = ( *entry )["letter"].get<sol::optional<std::string>>();
    out->entry_scope = ( *entry )["scope"].get<sol::optional<std::string>>();
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

// The keybindings screen was built to scroll a window over a list with nothing
// selected in it, and to refuse to scroll a list that already fits. Without
// sight that is not a small flaw: on most screens every arrow key answers with
// silence, which reads as a dead keyboard, and no row below the first can ever
// be reached. These cases pin the selection that replaces it.

TEST_CASE( "keybindings_selection_walks_a_list_that_fits_on_screen", "[lua]" )
{
    // Six rows, a window of twenty: upstream would not move at all here.
    cata::list_position at{ 0, 0 };

    at = cata::move_selection( at, 1, 6, 20, true );
    CHECK( at.selected == 1 );
    CHECK( at.offset == 0 );

    at = cata::move_selection( at, 1, 6, 20, true );
    CHECK( at.selected == 2 );
    // The window stays put while the row it holds is visible.
    CHECK( at.offset == 0 );
}

TEST_CASE( "keybindings_selection_drags_the_window_only_at_the_edges", "[lua]" )
{
    // Standing on the last visible row of a longer list.
    cata::list_position at{ 4, 0 };

    at = cata::move_selection( at, 1, 20, 5, true );
    CHECK( at.selected == 5 );
    CHECK( at.offset == 1 );

    // And back up the other way.
    at = cata::move_selection( at, -1, 20, 5, true );
    CHECK( at.selected == 4 );
    CHECK( at.offset == 1 );
}

TEST_CASE( "keybindings_selection_reaches_the_last_row", "[lua]" )
{
    // The tail of the list was unreachable before, since the offset stopped a
    // whole window short of the end.
    const cata::list_position at = cata::move_selection( { 18, 14 }, 1, 20, 5, true );
    CHECK( at.selected == 19 );
    CHECK( at.offset == 15 );
}

TEST_CASE( "keybindings_selection_wraps_a_step_and_clamps_a_page", "[lua]" )
{
    // A step off either end lands on the other, so a keypress is always
    // answered by a row rather than by silence.
    cata::list_position at = cata::move_selection( { 19, 15 }, 1, 20, 5, true );
    CHECK( at.selected == 0 );
    CHECK( at.offset == 0 );

    at = cata::move_selection( { 0, 0 }, -1, 20, 5, true );
    CHECK( at.selected == 19 );
    CHECK( at.offset == 15 );

    // A page is a distance rather than a step, so it stops at the end instead
    // of arriving at the far one.
    at = cata::move_selection( { 17, 13 }, 5, 20, 5, false );
    CHECK( at.selected == 19 );

    at = cata::move_selection( { 2, 0 }, -5, 20, 5, false );
    CHECK( at.selected == 0 );
}

TEST_CASE( "keybindings_selection_survives_the_filter_shrinking_the_list", "[lua]" )
{
    // Typing rewrites the list under the selection on every keypress.
    cata::list_position at = cata::clamp_selection( { 17, 13 }, 4, 5 );
    CHECK( at.selected == 3 );
    CHECK( at.offset == 0 );

    // A filter matching nothing leaves nothing to stand on, and the row the
    // hook would report has to be gone with it.
    at = cata::clamp_selection( { 3, 0 }, 0, 5 );
    CHECK( at.selected == 0 );
    CHECK( at.offset == 0 );

    at = cata::move_selection( { 0, 0 }, 1, 0, 5, true );
    CHECK( at.selected == 0 );
    CHECK( at.offset == 0 );
}

// The screen has to arrive whole: whose keys are being listed, what the screen
// is called, how many rows the filter left, and the row the selection sits on
// with its name and its keys. That is the same shape a menu arrives in, which is
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
    cata::fire_on_keybindings( ctxt, "INVENTORY", rows, { 0 } );

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

// The selection is the only thing saying which row a keypress landed on. Off by
// one here means every step announces its neighbour, which a player without
// sight has no way to catch.
TEST_CASE( "lua_hook_on_keybindings_reports_the_row_the_selection_points_at", "[lua]" )
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

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, { 1 } );
    REQUIRE( seen->cursor.has_value() );
    CHECK( *seen->cursor == 2 );
    REQUIRE( seen->has_entry );
    CHECK( seen->entry_text == "Wield an item" );

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, { 2 } );
    REQUIRE( seen->cursor.has_value() );
    CHECK( *seen->cursor == 3 );
    CHECK( seen->entry_text == "Close the window" );
}

// Typing a filter that matches nothing leaves the screen open on an empty list,
// and the selection it kept from before then points at nothing. Both params stay
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

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", {}, { 0 } );
    CHECK( seen->calls == 1 );
    CHECK( seen->count == 0 );
    CHECK_FALSE( seen->cursor.has_value() );
    CHECK_FALSE( seen->has_entry );

    // An offset left pointing past the end of a shorter list answers the same.
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, { 3 } );
    CHECK( seen->calls == 2 );
    CHECK( seen->count == 3 );
    CHECK_FALSE( seen->cursor.has_value() );
    CHECK_FALSE( seen->has_entry );
}

// A key is changed on this screen by pressing one of three keys and then the
// letter of a row -- letters the screen draws only in that mode and names
// nowhere. Reading the list without them is a list that cannot be used, so the
// letter comes over while it acts and stays away while it does not.
TEST_CASE( "lua_hook_on_keybindings_gives_the_letter_only_while_it_picks_a_row", "[lua]" )
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

    cata::keybindings_screen at;
    at.selected = 1;
    at.hotkeys = "abc";
    at.adds_local = '+';
    at.adds_global = '=';
    at.removes = '-';

    // Reading the list: the row's own key, and no letter, because pressing one
    // would go into the filter rather than choose anything.
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, at );
    CHECK_FALSE( seen->picking );
    REQUIRE( seen->has_entry );
    CHECK( seen->entry_column == ctxt.get_desc( "BN_TEST_TWO" ) );
    CHECK_FALSE( seen->entry_letter.has_value() );
    // The three keys that start a change are the screen's own, always carried,
    // since nothing else in the game ever says them.
    CHECK( seen->adds_local == "+" );
    CHECK( seen->adds_global == "=" );
    CHECK( seen->removes == "-" );

    // Changing a key: the second row is picked by the second letter.
    at.picking = true;
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, at );
    CHECK( seen->picking );
    REQUIRE( seen->entry_letter.has_value() );
    CHECK( *seen->entry_letter == "b" );

    // Scrolled down, the letters belong to the window and not to the list, so
    // the same row is picked by a different letter.
    at.selected = 2;
    at.offset = 2;
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, at );
    REQUIRE( seen->entry_letter.has_value() );
    CHECK( *seen->entry_letter == "a" );

    // A row past the end of the drawn letters has none, and says so by absence
    // rather than by handing over something that picks another row.
    at.hotkeys = "a";
    at.selected = 2;
    at.offset = 0;
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, at );
    CHECK_FALSE( seen->entry_letter.has_value() );
}

// Where a key works is drawn in colour on that screen -- green for this screen
// only, grey for the whole game -- and said nowhere. It is also the difference
// between the two keys that add one, so a change made without it is a guess.
TEST_CASE( "lua_hook_on_keybindings_says_where_a_key_works", "[lua]" )
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

    // An action the game itself binds, so this is asserted against real data
    // rather than against something the test set up.
    const std::vector<std::string> real{ "HELP_KEYBINDINGS" };
    REQUIRE( ctxt.get_desc( "HELP_KEYBINDINGS" ).find( "Unbound" ) == std::string::npos );

    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", real, { 0 } );
    REQUIRE( seen->has_entry );
    REQUIRE( seen->entry_scope.has_value() );
    CHECK( *seen->entry_scope == "global" );

    // Nothing to say about where a key works when there is no key: the second
    // column already says it is unbound, and a scope beside that is noise.
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, { 0 } );
    REQUIRE( seen->has_entry );
    CHECK_FALSE( seen->entry_scope.has_value() );
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
    cata::fire_on_keybindings( ctxt, "DEFAULTMODE", rows, { 0 } );
}
