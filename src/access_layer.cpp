#include "access_layer.h"

#include "catalua.h"
#include "catalua_hooks.h"
#include "catalua_impl.h"
#include "catalua_sol.h"
#include "debug.h"
#include "path_info.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace cata
{
namespace access
{

namespace
{

// The layer's entry point. Everything else it uses is required from there,
// relative to that path, so this is the only place the location is written down.
auto main_script() -> std::string
{
    return PATH_INFO::datadir() + "access/main.lua";
}

// Never freed, deliberately. The layer has to speak until the process is gone,
// so this state outlives the game's own teardown -- and a destructor running
// after that runs at CRT shutdown, against containers in other translation
// units whose destruction order the language does not define. That is a crash
// decided by link order rather than by anything here: silent, because the
// player has already quit, and able to disappear on the next build without a
// line changing. Owning it as a raw pointer removes the question. The
// operating system reclaims the memory.
lua_state *boot = nullptr;

} // namespace

void load_into( lua_state &state, bool at_boot )
{
    // Which of the two lives this is. The layer says different things: before a
    // world there is nothing loading and nothing to report about one, and the
    // only useful word is that the layer itself is there.
    state.lua.globals()["access_is_boot"] = at_boot;

    try {
        run_lua_script( state.lua, main_script() );
    } catch( const std::runtime_error &err ) {
        // Reported rather than rethrown: the game keeps running without speech
        // rather than not at all, and the report reaches the error screen, which
        // is the one screen the layer is not needed to speak.
        debugmsg( "Accessibility layer failed to load: %s", err.what() );
    }
}

void start()
{
    if( boot ) {
        return;
    }

    // release(), not the unique_ptr itself: the state is built through the
    // game's own factory and then handed over to nobody, for the reason above.
    boot = make_wrapped_state().release();
    // No mods, but the tables the layer expects still have to exist: this state
    // never goes through the mod system that would otherwise build them.
    init_global_state_tables( *boot, std::vector<mod_id> {} );
    define_hooks( *boot );

    load_into( *boot, true );
}

auto boot_state() -> lua_state *
{
    return boot;
}

} // namespace access

} // namespace cata
