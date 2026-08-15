#include "ui_hook.h"

#include "catalua_hooks.h"
#include "catalua_sol.h"
#include "init.h"
#include "ui.h"

namespace cata
{

void fire_on_uilist( const std::string &category, const std::string &title,
                     const std::string &text, const std::vector<uilist_entry> &entries,
                     const std::vector<int> &filtered, const int cursor )
{
    // The main menu is a uilist and is on screen before a world is loaded, when
    // there is no Lua state at all; `has_hooks` would dereference a null pointer.
    if( !DynamicDataLoader::get_instance().lua ) {
        return;
    }
    if( !has_hooks( "on_uilist" ) ) {
        return;
    }

    run_hooks( "on_uilist", [&]( sol::table & params ) {
        params["category"] = category;
        params["title"] = title;
        params["text"] = text;
        params["count"] = static_cast<int>( filtered.size() );

        // `filtered` holds indices into `entries`, and the menu's own cursor is
        // an index into `filtered`, which the menu sets to -1 while its filter
        // leaves nothing selectable.
        if( cursor < 0 || static_cast<size_t>( cursor ) >= filtered.size() ) {
            return;
        }
        const int index = filtered[cursor];
        if( index < 0 || static_cast<size_t>( index ) >= entries.size() ) {
            return;
        }
        const uilist_entry &current = entries[index];

        params["cursor"] = cursor + 1;
        sol::state_view lua( params.lua_state() );
        sol::table entry = lua.create_table( 0, 4 );
        entry["text"] = current.txt;
        entry["desc"] = current.desc;
        entry["column"] = current.ctxt;
        entry["enabled"] = current.enabled;
        params["entry"] = entry;
    } );
}

} // namespace cata
