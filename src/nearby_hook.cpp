#include "nearby_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "item.h"
#include "item_category.h"
#include "map_item_stack.h"

#include <string>

namespace cata
{

void fire_on_nearby_items( const std::vector<nearby_row> &rows, const std::string &action )
{
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_nearby_items" ) ) {
        return;
    }

    run_hooks( "on_nearby_items", [&]( sol::table & params ) {
        params["count"] = static_cast<int>( rows.size() );

        for( size_t i = 0; i < rows.size(); ++i ) {
            const nearby_row &row = rows[i];
            if( !row.selected ) {
                continue;
            }

            sol::state_view lua( params.lua_state() );
            sol::table entry = lua.create_table( 0, 6 );
            entry["text"] = row.text;
            entry["category"] = row.category;
            entry["count"] = row.count;
            // Plain numbers rather than a coordinate: the layer turns them into a
            // distance and a compass point, and nothing downstream wants a
            // position on a map it cannot look at.
            entry["dx"] = row.offset.x();
            entry["dy"] = row.offset.y();
            entry["dz"] = row.offset.z();

            params["cursor"] = static_cast<int>( i ) + 1;
            params["entry"] = entry;
            return;
        }
    } );
}

void fire_on_nearby_items( const std::vector<map_item_stack> &filtered,
                           const map_item_stack *active, const int page, const bool grouped,
                           const std::string &action )
{
    if( !has_hooks( "on_nearby_items" ) ) {
        return;
    }

    std::vector<nearby_row> rows;
    rows.reserve( filtered.size() );
    for( const map_item_stack &stack : filtered ) {
        if( stack.example == nullptr || stack.vIG.empty() ) {
            continue;
        }

        nearby_row row;
        // The name the screen prints in its own info title, so one item reads the
        // same here as on every other screen that lists it.
        row.text = stack.example->display_name( stack.totalcount );
        row.count = stack.totalcount;
        row.category = grouped ? stack.example->get_category().name() : std::string();
        row.selected = &stack == active;
        // The square the selection is currently pointing at, which for a stack
        // lying in several places is the one the left and right keys have paged to.
        // Every other row answers for the first of its places, which is the one the
        // screen draws its trail to.
        const int group = row.selected && page >= 0 &&
                          static_cast<size_t>( page ) < stack.vIG.size() ? page : 0;
        row.offset = stack.vIG[group].pos;
        rows.push_back( row );
    }

    fire_on_nearby_items( rows, action );
}

} // namespace cata