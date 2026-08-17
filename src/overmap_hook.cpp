#include "overmap_hook.h"

#include "access_places.h"
#include "avatar.h"
#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "omdata.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "regional_settings.h"

#include <string>
#include <tuple>

namespace cata
{

namespace
{

// How many tiles the previewed route holds, and 0 unless it leads to the tile
// the cursor is on. The player keeps one previewed route at a time, so a route
// asked for and then abandoned by moving the cursor elsewhere is still there to
// be read; reporting it against another tile would answer a question nobody
// asked. The path is stored destination first -- travelling pops its back -- so
// the front of it is where it goes.
int route_to( const avatar &you, const tripoint_abs_omt &cursor )
{
    if( you.omt_path.empty() || you.omt_path.front() != cursor ) {
        return 0;
    }
    return static_cast<int>( you.omt_path.size() );
}

// The player's own note, without the prefix she never typed. A note may begin
// with a symbol and a colour for the map to draw it with -- "X:" and "red;" --
// and the game's own parser says where the text starts, so the marks are not
// read out as words. Parsed by the game rather than here: a second parser for
// the same string is a second thing to keep in step.
std::string note_of( overmapbuffer &buffer, const tripoint_abs_omt &cursor )
{
    const std::string &note = buffer.note( cursor );
    if( note.empty() ) {
        return note;
    }
    const size_t offset = std::get<2>( overmap_ui::get_note_display_info( note ) );
    return offset < note.size() ? note.substr( offset ) : std::string();
}

} // namespace

void fire_on_overmap( const overmap_view &view )
{
    if( !has_hooks( "on_overmap" ) ) {
        return;
    }

    run_hooks( "on_overmap", [&view]( sol::table & params ) {
        params["place"] = view.place;
        params["note"] = view.note;
        params["seen"] = view.seen;
        params["explored"] = view.explored;
        params["dx"] = view.dx;
        params["dy"] = view.dy;
        params["dz"] = view.dz;
        params["route"] = view.route;
        params["action"] = view.action;
    } );
}

void fire_on_overmap( const tripoint_abs_omt &cursor, const std::string &action )
{
    if( action == "TIMEOUT" || action == "ERROR" || action == "ANY_INPUT" ) {
        return;
    }
    if( !has_hooks( "on_overmap" ) ) {
        return;
    }

    overmapbuffer &buffer = ACTIVE_OVERMAP_BUFFER;
    const avatar &you = get_avatar();
    const tripoint_abs_omt from = you.abs_omt_pos();

    overmap_view view;
    view.seen = buffer.seen( cursor );
    // A tile that has never been seen has no description to give, and the
    // sidebar prints none either. The debug vision trait, which the sidebar
    // does honour here, is not reproduced: it exists to look at the map from
    // outside the game, and a layer that spoke through it would be reporting
    // what the character cannot know.
    if( view.seen ) {
        view.place = access::place_description_at( cursor );
    }
    view.explored = buffer.is_explored( cursor );
    view.note = note_of( buffer, cursor );
    view.dx = cursor.x() - from.x();
    view.dy = cursor.y() - from.y();
    view.dz = cursor.z() - from.z();
    view.route = route_to( you, cursor );
    view.action = action;

    fire_on_overmap( view );
}

} // namespace cata
