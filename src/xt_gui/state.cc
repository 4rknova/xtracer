#include "config.h"
#include "state.h"

namespace gui {

textures_t::textures_t()
    : logo(0)
{}

window_t::window_t()
    : width(WINDOW_DEFAULT_WIDTH)
    , height(WINDOW_DEFAULT_HEIGHT)
{}

state_t::state_t()
    : workspace(0)
{}

} /* namespace gui */
