#include "nearby_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "character.h"
#include "creature.h"
#include "item.h"
#include "item_category.h"
#include "map_item_stack.h"
#include "translations.h"

#include <string>

namespace cata
{

namespace
{

// Handing the rows over, for whichever of the two lists asked. The two screens are
// one screen with two tabs and are read by one model, so they differ in the hook
// name and in nothing else.
//
// Called from a `run_hooks` that spells its hook name out, rather than taking the
// name itself, because the map of the job in `werk\coverage.py` finds a firing
// point by looking for exactly that literal: a hook fired through a variable is a
// hook the drift check reports as gone the next time upstream is merged.
void write_rows( sol::table &params, const std::vector<nearby_row> &rows )
{
    {
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
    }
}

} // namespace

void fire_on_nearby_items( const std::vector<nearby_row> &rows, const std::string &action )
{
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_nearby_items" ) ) {
        return;
    }

    run_hooks( "on_nearby_items", [&]( sol::table & params ) {
        write_rows( params, rows );
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

void fire_on_nearby_monsters( const std::vector<Creature *> &monsters, const int cursor,
                              const std::string &action )
{
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_nearby_monsters" ) ) {
        return;
    }

    const Character &you = get_player_character();

    std::vector<nearby_row> rows;
    rows.reserve( monsters.size() );
    for( size_t i = 0; i < monsters.size(); ++i ) {
        const Creature *critter = monsters[i];
        if( critter == nullptr ) {
            continue;
        }

        nearby_row row;
        row.text = critter->disp_name();
        // How it stands towards her, in the game's own words. The screen draws this
        // as a colour and as the order it sorts by, and a name alone does not say
        // it: two of the same creature can differ, and it is what decides whether
        // the answer is to walk over or to walk away.
        row.category = Creature::get_attitude_ui_data( critter->attitude_to( you ) ).first.translated();
        row.offset = critter->bub_pos() - you.bub_pos();
        row.selected = static_cast<int>( i ) == cursor;
        rows.push_back( row );
    }

    run_hooks( "on_nearby_monsters", [&]( sol::table & params ) {
        write_rows( params, rows );
    } );
}

} // namespace cata