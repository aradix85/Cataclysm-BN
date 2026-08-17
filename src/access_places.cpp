#include "access_places.h"

#include "map.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "regional_settings.h"

#include <string>

namespace cata::access
{

std::string area_name_at( const tripoint_abs_omt &p )
{
    return ACTIVE_OVERMAP_BUFFER.ter( p )->get_name();
}

std::string area_name_at( const tripoint_bub_ms &p )
{
    return area_name_at( project_to<coords::omt>( bub_to_abs( p ) ) );
}

std::string place_description_at( const tripoint_abs_omt &p )
{
    overmapbuffer &buffer = ACTIVE_OVERMAP_BUFFER;
    const oter_id here = buffer.ter( p );
    const regional_settings &region = buffer.get_settings( p );

    if( !region.display_oter.is_empty() && here == region.default_oter.id() ) {
        return region.display_oter.id().obj().get_name();
    }
    return buffer.get_description_at( project_to<coords::sm>( p ) );
}

} // namespace cata::access
