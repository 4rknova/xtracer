#include "macro.h"
#include "surface.h"
#include <cmath>

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

Vector3f ISurface::emitter_position() const
{
    if (std::isfinite((double)aabb.min.x) && std::isfinite((double)aabb.min.y) && std::isfinite((double)aabb.min.z) &&
        std::isfinite((double)aabb.max.x) && std::isfinite((double)aabb.max.y) && std::isfinite((double)aabb.max.z)) {
        return (aabb.min + aabb.max) * 0.5f;
    }
    return point_sample();
}

    } /* namespace asset */
} /* namespace xtcore */
