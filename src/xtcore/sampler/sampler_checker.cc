#include <cmath>

#include "sampler_checker.h"

#define XTCORE_CHECKER_DEFAULT_SCALE_U ((nmath::scalar_t)2.0)
#define XTCORE_CHECKER_DEFAULT_SCALE_V ((nmath::scalar_t)2.0)

namespace xtcore {
    namespace sampler {

Checker::Checker()
    : color_a(nimg::ColorRGBf(0.92f, 0.92f, 0.92f))
    , color_b(nimg::ColorRGBf(0.08f, 0.08f, 0.08f))
    , scale_u(XTCORE_CHECKER_DEFAULT_SCALE_U)
    , scale_v(XTCORE_CHECKER_DEFAULT_SCALE_V)
    , offset_u((nmath::scalar_t)0.0)
    , offset_v((nmath::scalar_t)0.0)
    , swap_colors(false)
{}

Checker::~Checker()
{}

nimg::ColorRGBf Checker::sample(const nmath::Vector3f &uvw) const
{
    // Treat near-zero scales as invalid to avoid collapsing the checker pattern
    // into a single constant value. The fallback keeps the texture meaningful.
    const nmath::scalar_t su = (std::fabs((double)scale_u) > 1e-6) ? scale_u : XTCORE_CHECKER_DEFAULT_SCALE_U;
    const nmath::scalar_t sv = (std::fabs((double)scale_v) > 1e-6) ? scale_v : XTCORE_CHECKER_DEFAULT_SCALE_V;

    // Convert incoming UV space into checker-cell space:
    // - scale controls cell density (tiles per unit)
    // - offset slides the checker grid in each axis
    const nmath::scalar_t u = uvw.x * su + offset_u;
    const nmath::scalar_t v = uvw.y * sv + offset_v;

    // Identify the integer checker cell containing the sample.
    // floor() is important for stable behavior across negative coordinates.
    const int iu = (int)std::floor((double)u);
    const int iv = (int)std::floor((double)v);

    // Classic checker parity test:
    // even (iu + iv) -> color_a, odd -> color_b.
    // Bitwise AND with 1 is a fast odd/even check.
    const bool odd = (((iu + iv) & 1) != 0) ^ swap_colors;
    return odd ? color_b : color_a;
}

    } /* namespace sampler */
} /* namespace xtcore */

#undef XTCORE_CHECKER_DEFAULT_SCALE_U
#undef XTCORE_CHECKER_DEFAULT_SCALE_V
