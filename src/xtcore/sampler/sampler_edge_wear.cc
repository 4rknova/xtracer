#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_edge_wear.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline uint32_t uhash(int x, int y, int seed)
{
    uint32_t h = (uint32_t)seed ^ ((uint32_t)x * 0x9e3779b9u) ^ ((uint32_t)y * 0x85ebca6bu);
    h ^= (h >> 16); h *= 0x45d9f3bu; h ^= (h >> 16);
    return h;
}

inline nmath::scalar_t h01(int x, int y, int seed, uint32_t ch)
{
    return (nmath::scalar_t)(uhash(x, y, seed) ^ ((ch * 0xc2b2ae35u) & 0x00ffffffu)) / (nmath::scalar_t)0x01000000u;
}

// Voronoi F2-F1 distance: returns distance to nearest cell boundary.
// High values = cell interior, low values = near a boundary / edge.
inline nmath::scalar_t voronoi_edge_dist(nmath::scalar_t u, nmath::scalar_t v, int seed)
{
    const int iu = (int)std::floor((double)u);
    const int iv = (int)std::floor((double)v);

    nmath::scalar_t f1 = (nmath::scalar_t)1e30;
    nmath::scalar_t f2 = (nmath::scalar_t)1e30;

    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int cx = iu + dx;
            const int cy = iv + dy;
            const nmath::scalar_t jx = h01(cx, cy, seed, 0u);
            const nmath::scalar_t jy = h01(cx, cy, seed, 1u);
            const nmath::scalar_t px = (nmath::scalar_t)cx + jx - u;
            const nmath::scalar_t py = (nmath::scalar_t)cy + jy - v;
            const nmath::scalar_t d  = px * px + py * py;
            if (d < f1) { f2 = f1; f1 = d; }
            else if (d < f2) { f2 = d; }
        }
    }

    return (nmath::scalar_t)std::sqrt((double)f2) - (nmath::scalar_t)std::sqrt((double)f1);
}

} // namespace

EdgeWear::EdgeWear()
    : color_base(nimg::ColorRGBf(0.50f, 0.48f, 0.46f))
    , color_worn(nimg::ColorRGBf(0.92f, 0.90f, 0.88f))
    , scale((nmath::scalar_t)8.0)
    , sharpness((nmath::scalar_t)6.0)
    , coverage((nmath::scalar_t)0.5)
    , seed(1337)
{}

EdgeWear::~EdgeWear()
{}

nimg::ColorRGBf EdgeWear::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)8.0;
    const nmath::scalar_t u = uvw.x * s;
    const nmath::scalar_t v = uvw.y * s;

    const nmath::scalar_t edge = voronoi_edge_dist(u, v, seed);

    // Low edge distance = near a boundary = worn.  Map with a power curve for sharpness.
    const nmath::scalar_t cov = clamp01(coverage);
    const nmath::scalar_t sp  = (sharpness > (nmath::scalar_t)0.1) ? sharpness : (nmath::scalar_t)0.1;
    const nmath::scalar_t raw = clamp01(edge / (cov + (nmath::scalar_t)1e-6));
    const nmath::scalar_t t   = (nmath::scalar_t)1.0 - (nmath::scalar_t)std::pow((double)raw, (double)sp);

    return color_base * ((nmath::scalar_t)1.0 - t) + color_worn * t;
}

    } /* namespace sampler */
} /* namespace xtcore */
