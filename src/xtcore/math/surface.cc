#include "macro.h"
#include "surface.h"

namespace xtcore {
    namespace asset {

ISurface::ISurface()
	: uv_scale(Vector2f(1,1))
{}

ISurface::~ISurface()
{}

nmath::scalar_t ISurface::distance(nmath::Vector3f p) const
{
    UNUSED(p)
    return INFINITY;
}

    } /* namespace asset */
} /* namespace xtcore */
