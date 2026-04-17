#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_brick.h"

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

inline nmath::scalar_t hash_brick(int row, int col, int seed)
{
    uint32_t h = (uint32_t)seed ^ ((uint32_t)row * 0x9e3779b9u) ^ ((uint32_t)col * 0x85ebca6bu);
    h ^= (h >> 16); h *= 0x45d9f3bu; h ^= (h >> 16);
    return (nmath::scalar_t)(h & 0x00ffffffu) / (nmath::scalar_t)0x01000000u;
}

} // namespace

Brick::Brick()
    : color_brick(nimg::ColorRGBf(0.72f, 0.32f, 0.22f))
    , color_mortar(nimg::ColorRGBf(0.72f, 0.70f, 0.66f))
    , scale_u((nmath::scalar_t)8.0)
    , scale_v((nmath::scalar_t)4.0)
    , mortar_u((nmath::scalar_t)0.06)
    , mortar_v((nmath::scalar_t)0.10)
    , color_variation((nmath::scalar_t)0.08)
    , seed(42)
{}

Brick::~Brick()
{}

nimg::ColorRGBf Brick::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t su = (std::fabs((double)scale_u) > 1e-6) ? scale_u : (nmath::scalar_t)8.0;
    const nmath::scalar_t sv = (std::fabs((double)scale_v) > 1e-6) ? scale_v : (nmath::scalar_t)4.0;

    const nmath::scalar_t u = uvw.x * su;
    const nmath::scalar_t v = uvw.y * sv;

    const int row = (int)std::floor((double)v);
    // Half-brick stagger on odd rows
    const nmath::scalar_t offset = (row & 1) ? (nmath::scalar_t)0.5 : (nmath::scalar_t)0.0;
    const int col = (int)std::floor((double)(u + offset));

    const nmath::scalar_t fu = fract(u + offset);
    const nmath::scalar_t fv = fract(v);

    const nmath::scalar_t half_mu = clamp01(mortar_u) * (nmath::scalar_t)0.5;
    const nmath::scalar_t half_mv = clamp01(mortar_v) * (nmath::scalar_t)0.5;

    const bool in_mortar = (fu < half_mu) || (fu > (nmath::scalar_t)1.0 - half_mu) ||
                           (fv < half_mv) || (fv > (nmath::scalar_t)1.0 - half_mv);

    if (in_mortar) return color_mortar;

    const nmath::scalar_t var = hash_brick(row, col, seed) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
    const nmath::scalar_t cv  = color_variation * var;
    return nimg::ColorRGBf(
        clamp01(color_brick.r() + cv),
        clamp01(color_brick.g() + cv),
        clamp01(color_brick.b() + cv)
    );
}

    } /* namespace sampler */
} /* namespace xtcore */
