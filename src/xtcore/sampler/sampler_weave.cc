#include <cmath>
#include <algorithm>

#include "sampler_weave.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline nimg::ColorRGBf shade_strand(const nimg::ColorRGBf &c, nmath::scalar_t local, nmath::scalar_t half_band, nmath::scalar_t visibility)
{
    const nmath::scalar_t denom = (half_band > (nmath::scalar_t)1e-6) ? half_band : (nmath::scalar_t)1.0;
    const nmath::scalar_t center = clamp01((nmath::scalar_t)std::fabs((double)(local - (nmath::scalar_t)0.5)) / denom);
    const nmath::scalar_t profile = (nmath::scalar_t)1.0 - center;
    const nmath::scalar_t shade = visibility * ((nmath::scalar_t)0.78 + (nmath::scalar_t)0.22 * profile);
    return c * shade;
}

} // namespace

Weave::Weave()
    : base_color(nimg::ColorRGBf(0.14f, 0.13f, 0.12f))
    , warp_color(nimg::ColorRGBf(0.86f, 0.80f, 0.72f))
    , weft_color(nimg::ColorRGBf(0.28f, 0.62f, 0.68f))
    , scale((nmath::scalar_t)12.0)
    , band_width((nmath::scalar_t)0.66)
{}

Weave::~Weave()
{}

nimg::ColorRGBf Weave::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)12.0;
    const nmath::scalar_t width = std::max((nmath::scalar_t)0.02, std::min((nmath::scalar_t)0.98, band_width));
    const nmath::scalar_t half_band = (nmath::scalar_t)0.5 * width;

    const nmath::scalar_t u = uvw.x * s;
    const nmath::scalar_t v = uvw.y * s;

    const int iu = (int)std::floor((double)u);
    const int iv = (int)std::floor((double)v);
    const nmath::scalar_t fu = u - (nmath::scalar_t)iu;
    const nmath::scalar_t fv = v - (nmath::scalar_t)iv;

    const bool on_warp = (std::fabs((double)(fu - (nmath::scalar_t)0.5)) <= half_band);
    const bool on_weft = (std::fabs((double)(fv - (nmath::scalar_t)0.5)) <= half_band);

    if (!on_warp && !on_weft) return base_color;

    const bool warp_on_top = (((iu + iv) & 1) == 0);
    const nmath::scalar_t below_visibility = (nmath::scalar_t)0.62;

    if (on_warp && on_weft) {
        if (warp_on_top) return shade_strand(warp_color, fu, half_band, (nmath::scalar_t)1.0);
        return shade_strand(weft_color, fv, half_band, (nmath::scalar_t)1.0);
    }

    if (on_warp) {
        return shade_strand(warp_color, fu, half_band, warp_on_top ? (nmath::scalar_t)1.0 : below_visibility);
    }

    return shade_strand(weft_color, fv, half_band, warp_on_top ? below_visibility : (nmath::scalar_t)1.0);
}

    } /* namespace sampler */
} /* namespace xtcore */
