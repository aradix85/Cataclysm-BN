#include "inventory_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_category.h"

namespace cata
{

// Where an item is, in the words the Lua side reads. Only the places an
// inventory screen can show are named; anything else is left blank rather than
// guessed at, because a wrong place is worse than none.
static std::string place_of( const item &it )
{
    switch( it.where() ) {
        case item_location_type::character:
            return "character";
        case item_location_type::map:
            return "map";
        case item_location_type::vehicle:
            return "vehicle";
        case item_location_type::container:
            return "container";
        default:
            return std::string();
    }
}

void fire_on_inventory( const std::string &title, const std::vector<inventory_line> &lines )
{
    if( !has_hooks( "on_inventory" ) ) {
        return;
    }

    run_hooks( "on_inventory", [&]( sol::table & params ) {
        params["title"] = title;
        params["count"] = static_cast<int>( lines.size() );

        for( size_t i = 0; i < lines.size(); ++i ) {
            const inventory_line &line = lines[i];
            if( !line.selected ) {
                continue;
            }

            params["cursor"] = static_cast<int>( i ) + 1;
            sol::state_view lua( params.lua_state() );
            sol::table entry = lua.create_table( 0, 6 );
            entry["text"] = line.text;
            entry["category"] = line.category;
            entry["where"] = line.where;
            entry["denial"] = line.denial;
            entry["enabled"] = line.enabled;
            entry["marked"] = line.marked;
            params["entry"] = entry;
            return;
        }
    } );
}

void fire_on_inventory( inventory_selector &selector )
{
    if( !has_hooks( "on_inventory" ) ) {
        return;
    }

    // The active column only. The screen has several -- the items, and on the
    // marking screens a column of what is already marked -- but exactly one has
    // the cursor, and that is the list the player is walking.
    inventory_column &column = selector.get_active_column();
    const inventory_selector_preset &preset = selector.get_preset();
    // Compared by address, not by value: two rows of the same item read
    // identically, and the entries the column hands out live in the column
    // itself, so the selected one is the same object as the row that carries it.
    const inventory_entry &selected = column.get_selected();

    // `get_entries` walks what the screen currently shows; `get_all_entries`
    // would add the rows the filter is hiding, and speaking a position out of a
    // list the player cannot step through is worse than saying nothing.
    const std::vector<inventory_entry *> shown =
    column.get_entries( []( const inventory_entry & entry ) {
        return entry.is_item();
    } );

    std::vector<inventory_line> lines;
    lines.reserve( shown.size() );
    for( const inventory_entry *entry : shown ) {
        inventory_line line;
        line.text = preset.get_cell_text( *entry, 0 );
        const item_category *category = entry->get_category_ptr();
        line.category = category != nullptr ? category->name() : std::string();
        line.where = place_of( *entry->any_item() );
        line.denial = preset.get_denial( *entry );
        line.marked = static_cast<int>( entry->chosen_count );
        line.enabled = entry->is_selectable();
        line.selected = entry == &selected;
        lines.push_back( line );
    }

    fire_on_inventory( selector.get_title(), lines );
}

} // namespace cata
