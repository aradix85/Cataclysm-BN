#include "morale_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"

#include <cstddef>

namespace cata
{

std::vector<morale_row> morale_summary_rows( const morale_summary &at )
{
    std::vector<morale_row> rows;

    // Only where they apply, which is what the screen does with them: a
    // character in no pain is not owed a line saying so.
    if( at.pain_penalty != 0 ) {
        rows.push_back( morale_row{ morale_label_text( _( "Pain level:" ) ),
                                    morale_total_text( -at.pain_penalty ) } );
    }
    if( at.fatigue_cap != 0 ) {
        rows.push_back( morale_row{ morale_label_text( _( "Fatigue Morale Cap:" ) ),
                                    string_format( "%d", at.fatigue_cap ) } );
    }
    rows.push_back( morale_row{ morale_label_text( _( "Focus trends towards:" ) ),
                                string_format( "%d", at.focus ) } );
    return rows;
}

// A list of rows as Lua reads them: one table per row, in order, from one.
static sol::table row_list( sol::state_view lua, const std::vector<morale_row> &rows )
{
    sol::table list = lua.create_table( static_cast<int>( rows.size() ), 0 );
    for( std::size_t i = 0; i < rows.size(); ++i ) {
        sol::table row = lua.create_table( 0, 2 );
        row["text"] = rows[i].text;
        row["value"] = rows[i].value;
        list[i + 1] = row;
    }
    return list;
}

void fire_on_morale( const std::vector<morale_row> &rows, const morale_summary &at,
                     const std::string &action )
{
    if( !has_hooks( "on_morale" ) ) {
        return;
    }

    run_hooks( "on_morale", [&]( sol::table & params ) {
        sol::state_view lua( params.lua_state() );

        params["rows"] = row_list( lua, rows );
        params["summary"] = row_list( lua, morale_summary_rows( at ) );
        params["total"] = morale_total_text( at.total );
        // The same string the screen draws above the list, so it is called what
        // the game calls it.
        params["title"] = _( "Morale" );

        // What the previous round of this screen's own loop answered, so the
        // reading position can follow the arrow keys the screen itself does
        // nothing with. Empty means this is the screen opening.
        params["action"] = action;
    } );
}

} // namespace cata
