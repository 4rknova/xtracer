#include <cmath>
#include <algorithm>

#include "sampler_graphpaper.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline int positive_mod(int a, int b)
{
    if (b <= 0) return 0;
    int m = a % b;
    if (m < 0) m += b;
    return m;
}

} // namespace

GraphPaper::GraphPaper()
    : base_color(nimg::ColorRGBf(0.93f, 0.93f, 0.93f))
    , minor_color(nimg::ColorRGBf(0.71f, 0.83f, 0.89f))
    , major_color(nimg::ColorRGBf(0.30f, 0.63f, 0.77f))
    , scale((nmath::scalar_t)16.0)
    , minor_width((nmath::scalar_t)0.020)
    , major_width((nmath::scalar_t)0.050)
    , major_every(5)
{}

GraphPaper::~GraphPaper()
{}

nimg::ColorRGBf GraphPaper::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (scale > (nmath::scalar_t)EPSILON) ? scale : (nmath::scalar_t)16.0;
    const nmath::scalar_t u = uvw.x * s;
    const nmath::scalar_t v = uvw.y * s;

    const int iu = (int)std::floor((double)u);
    const int iv = (int)std::floor((double)v);
    const nmath::scalar_t fu = u - (nmath::scalar_t)iu;
    const nmath::scalar_t fv = v - (nmath::scalar_t)iv;

    const nmath::scalar_t mw = clamp01(minor_width) * (nmath::scalar_t)0.5;
    const nmath::scalar_t Mw = clamp01(major_width) * (nmath::scalar_t)0.5;

    const bool on_u_minor = (fu <= mw) || (fu >= ((nmath::scalar_t)1.0 - mw));
    const bool on_v_minor = (fv <= mw) || (fv >= ((nmath::scalar_t)1.0 - mw));
    const bool on_minor = on_u_minor || on_v_minor;

    const int every = (major_every < 1) ? 1 : major_every;
    const bool major_u_cell = (positive_mod(iu, every) == 0);
    const bool major_v_cell = (positive_mod(iv, every) == 0);
    const bool on_u_major = major_u_cell && ((fu <= Mw) || (fu >= ((nmath::scalar_t)1.0 - Mw)));
    const bool on_v_major = major_v_cell && ((fv <= Mw) || (fv >= ((nmath::scalar_t)1.0 - Mw)));
    const bool on_major = on_u_major || on_v_major;

    if (on_major) return major_color;
    if (on_minor) return minor_color;
    return base_color;
}

    } /* namespace sampler */
} /* namespace xtcore */
