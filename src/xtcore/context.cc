#include "context.h"

namespace xtracer {
    namespace render {

context_t::context_t()
    : scene(0)
    , framebuffer(0)
    , threads(0)
    , samples_ssaa(1)
    , samples_reflection(1)
    , samples_shadow(1)
    , samples_dof(1)
{}

    } /* namespace render */
} /* namespace xtracer */
