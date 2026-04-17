#include <cmath>
#include <algorithm>

#include "sampler_dots.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline nmath::scalar_t fract(nmath::scalar_t v)
{
    return v - (nmath::scalar_t)std::floor((double)v);
}

} // namespace

Dots::Dots()
    : color_bg(nimg::ColorRGBf(0.92f, 0.92f, 0.92f))
    , color_dot(nimg::ColorRGBf(0.10f, 0.10f, 0.10f))
    , scale((nmath::scalar_t)8.0)
    , radius((nmath::scalar_t)0.35)
    , softness((nmath::scalar_t)0.05)
{}

Dots::~Dots()
{}

nimg::ColorRGBf Dots::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)8.0;

    const nmath::scalar_t u = uvw.x * s;
    const nmath::scalar_t v = uvw.y * s;

    // Local position within the unit cell [0,1)^2
    const nmath::scalar_t fu = fract(u) - (nmath::scalar_t)0.5;
    const nmath::scalar_t fv = fract(v) - (nmath::scalar_t)0.5;
    const nmath::scalar_t d  = (nmath::scalar_t)std::sqrt((double)(fu * fu + fv * fv));

    const nmath::scalar_t r = clamp01(radius);
    const nmath::scalar_t soft = clamp01(softness) + (nmath::scalar_t)1e-6;
    const nmath::scalar_t t = clamp01(((nmath::scalar_t)1.0 - (d - r) / soft));

    return color_bg * ((nmath::scalar_t)1.0 - t) + color_dot * t;
}

    } /* namespace sampler */
} /* namespace xtcore */
