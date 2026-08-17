#include "advanced_inventory_hook.h"

#include "advanced_inv_listitem.h"
#include "advanced_inv_pane.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "item.h"
#include "item_category.h"
#include "string_formatter.h"
#include "vehicle.h"

namespace cata
{

// Where a pane is aimed, in the words the screen draws above it: the vehicle's
// name when the pane is showing a vehicle's cargo, and the direction always,
// since the direction is what says where to walk and the screen replaces it.
static advanced_inventory_place place_of( const advanced_inventory_pane &pane,
        const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares )
{
    const advanced_inv_area &square = squares[pane.get_area()];

    advanced_inventory_place place;
    place.area = square.name;
    if( square.can_store_in_vehicle() && pane.in_vehicle() && square.veh != nullptr ) {
        place.vehicle = square.veh->name;
    }
    return place;
}

// The row in the words the plain inventory uses for the same thing, so that one
// item reads the same on every screen in the game that lists it. The screen
// itself draws the size of the stack in a column of its own, which is a
// position on a screen and therefore nothing at all without sight.
static std::string caption_of( const advanced_inv_listitem &entry )
{
    const std::string name = entry.items.front()->display_name( entry.stacks );
    return entry.stacks > 1 ? string_format( "%d %s", entry.stacks, name ) : name;
}

void fire_on_advanced_inventory( const advanced_inventory_place &source,
                                 const advanced_inventory_place &destination,
                                 const std::vector<advanced_inventory_line> &lines )
{
    if( !has_hooks( "on_advanced_inventory" ) ) {
        return;
    }

    run_hooks( "on_advanced_inventory", [&]( sol::table & params ) {
        sol::state_view lua( params.lua_state() );

        for( const auto &side : {
                 std::make_pair( "source", &source ), std::make_pair( "destination", &destination )
             } ) {
            sol::table place = lua.create_table( 0, 2 );
            place["area"] = side.second->area;
            place["vehicle"] = side.second->vehicle;
            params[side.first] = place;
        }

        params["count"] = static_cast<int>( lines.size() );

        for( size_t i = 0; i < lines.size(); ++i ) {
            const advanced_inventory_line &line = lines[i];
            if( !line.selected ) {
                continue;
            }

            params["cursor"] = static_cast<int>( i ) + 1;
            sol::table entry = lua.create_table( 0, 3 );
            entry["text"] = line.text;
            entry["category"] = line.category;
            entry["square"] = line.square;
            params["entry"] = entry;
            return;
        }
    } );
}

void fire_on_advanced_inventory( const advanced_inventory_pane &source,
                                 const advanced_inventory_pane &destination,
                                 const std::array<advanced_inv_area, NUM_AIM_LOCATIONS> &squares )
{
    if( !has_hooks( "on_advanced_inventory" ) ) {
        return;
    }

    // The screen inserts a category header at the top of each page, and only
    // when it is sorted by category. Sorted any other way there are no headings
    // to sit under, and a heading is not invented for a list that has none.
    const bool grouped = source.sortby == SORTBY_CATEGORY;
    const bool everywhere = source.get_area() == AIM_ALL;

    std::vector<advanced_inventory_line> lines;
    lines.reserve( source.items.size() );
    for( size_t i = 0; i < source.items.size(); ++i ) {
        const advanced_inv_listitem &entry = source.items[i];
        // Headers and the blank entry that pads a page are drawn as rows and
        // cannot be chosen, so they are neither counted nor spoken.
        if( !entry.is_item_entry() ) {
            continue;
        }

        advanced_inventory_line line;
        line.text = caption_of( entry );
        line.category = grouped && entry.cat != nullptr ? entry.cat->name() : std::string();
        line.square = everywhere ? squares[entry.area].name : std::string();
        line.selected = static_cast<int>( i ) == source.index;
        lines.push_back( line );
    }

    fire_on_advanced_inventory( place_of( source, squares ), place_of( destination, squares ), lines );
}

} // namespace cata
