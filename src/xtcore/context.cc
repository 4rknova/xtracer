#include "context.h"

namespace xtracer {
    namespace render {

params_t::params_t()
    : threads(0)
    , samples(1)
    , ssaa(1)
    , rdepth(3)
    , tile_size(64)
{}

context_t::context_t()
    : scene(0)
    , framebuffer(0)
{}

    } /* namespace render */
} /* namespace xtracer */
