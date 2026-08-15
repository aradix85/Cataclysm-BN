#include "main_menu_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "color.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "world.h"
#include "worldfactory.h"

#include <cstddef>

namespace cata
{

namespace
{

// The headings, in the order `main_menu_opts` declares them in
// src/main_menu.cpp. Which list hangs under which heading is decided there, in
// `main_menu::display_sub_menu`, and is mirrored here rather than read from it:
// those lists are private to `main_menu`, and the two that are fixed strings
// are handed over by the call site instead. If upstream reorders that enum,
// this follows it.
constexpr int opt_new_char = 1;
constexpr int opt_load_char = 2;
constexpr int opt_world = 3;
constexpr int opt_settings = 4;

// What is drawn, as something that can be said. A menu item carries its hotkey
// as markup around the letter that is highlighted -- `Se<t|T>tings` -- and the
// game's own function is what resolves it, so the name here is the name on
// screen and no second spelling of it exists.
auto spoken( const std::string &drawn ) -> std::string
{
    return remove_color_tags( shortcut_text( c_white, drawn ) );
}

// The key that jumps straight to what is drawn, or nothing where there is none.
//
// The game marks two or three per name -- `H<e|E|?>lp` -- and highlights the
// first, which is the one a sighted player sees and the only one worth saying.
// A world name carries no marking at all: worlds are reached with the arrow keys.
auto hotkey_of( const std::string &drawn ) -> std::string
{
    const std::vector<std::string> keys = get_hotkeys( drawn );
    return keys.empty() ? std::string() : keys.front();
}

// One line of the list under a heading, as the game draws it.
//
// `saves` is how many characters live in a world, which the screen draws in
// brackets after its name -- "Boston (2)". It comes over as a number of its own
// rather than inside the name, because a nought there is the difference between
// a world that can be entered and one that answers Return with a refusal, and
// because two numbers in one breath -- "Boston 2, 1 of 3" -- read as related
// when they are not. Below zero for anything that is not a world.
struct menu_entry {
    std::string drawn;
    int saves = -1;
};

// The worlds, each with how many characters live in it.
auto world_items() -> std::vector<menu_entry>
{
    std::vector<menu_entry> out;
    for( const std::string &name : world_generator->all_worldnames() ) {
        const WORLDINFO *world = world_generator->get_world( name );
        out.push_back( { name, world ? static_cast<int>( world->world_saves.size() ) : 0 } );
    }
    return out;
}

// The list drawn under the selected heading, as the game writes it and with its
// markup intact, or nothing where the heading carries no list.
auto entries_under( const int heading, const std::vector<std::string> &new_game_items,
                    const std::vector<std::string> &settings_items ) -> std::vector<menu_entry>
{
    std::vector<menu_entry> out;
    switch( heading ) {
        case opt_new_char:
        case opt_settings: {
            const std::vector<std::string> &items =
                heading == opt_new_char ? new_game_items : settings_items;
            out.reserve( items.size() );
            for( const std::string &item : items ) {
                out.push_back( { item, -1 } );
            }
            return out;
        }
        case opt_load_char:
            return world_items();
        case opt_world:
            // The world screen offers making one before listing the ones there are.
            out = world_items();
            out.insert( out.begin(), { _( "Create World" ), -1 } );
            return out;
        default:
            return out;
    }
}

} // namespace

void fire_on_main_menu( const std::vector<std::string> &headings, const int heading_sel,
                        const std::vector<std::string> &new_game_items,
                        const std::vector<std::string> &settings_items, const int entry_sel )
{
    // No check for a world's Lua state: this screen is where there is never one,
    // and `resolve_hook_state` answers with the layer's own state there.
    if( !has_hooks( "on_main_menu" ) ) {
        return;
    }
    if( heading_sel < 0 || static_cast<std::size_t>( heading_sel ) >= headings.size() ) {
        return;
    }

    const std::vector<menu_entry> entries =
        entries_under( heading_sel, new_game_items, settings_items );

    run_hooks( "on_main_menu", [&]( sol::table & params ) {
        sol::state_view lua( params.lua_state() );
        params["category"] = "MAIN_MENU";

        sol::table heading = lua.create_table( 0, 4 );
        heading["text"] = spoken( headings[heading_sel] );
        heading["key"] = hotkey_of( headings[heading_sel] );
        heading["cursor"] = heading_sel + 1;
        heading["count"] = static_cast<int>( headings.size() );
        params["heading"] = heading;

        if( entry_sel < 0 || static_cast<std::size_t>( entry_sel ) >= entries.size() ) {
            return;
        }

        sol::table entry = lua.create_table( 0, 5 );
        entry["text"] = spoken( entries[entry_sel].drawn );
        entry["key"] = hotkey_of( entries[entry_sel].drawn );
        entry["cursor"] = entry_sel + 1;
        entry["count"] = static_cast<int>( entries.size() );
        if( entries[entry_sel].saves >= 0 ) {
            entry["saves"] = entries[entry_sel].saves;
        }
        params["entry"] = entry;
    } );
}

} // namespace cata
