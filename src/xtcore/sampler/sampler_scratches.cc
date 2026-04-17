#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_scratches.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline uint32_t uhash(uint32_t a, uint32_t b)
{
    uint32_t h = a * 0x9e3779b9u ^ b * 0x85ebca6bu;
    h ^= (h >> 16); h *= 0x45d9f3bu; h ^= (h >> 16);
    return h;
}

inline nmath::scalar_t h01(uint32_t a, uint32_t b)
{
    return (nmath::scalar_t)(uhash(a, b) & 0x00ffffffu) / (nmath::scalar_t)0x01000000u;
}

// Signed distance from point (px,py) to a line segment defined by two unit-direction
// vectors of given half-length, centred at (cx,cy). Returns perpendicular distance.
inline nmath::scalar_t seg_dist(nmath::scalar_t px, nmath::scalar_t py,
                                nmath::scalar_t cx, nmath::scalar_t cy,
                                nmath::scalar_t dx, nmath::scalar_t dy,
                                nmath::scalar_t half_len)
{
    const nmath::scalar_t ex = px - cx;
    const nmath::scalar_t ey = py - cy;
    const nmath::scalar_t along = ex * dx + ey * dy;
    const nmath::scalar_t clamped = std::max(-half_len, std::min(half_len, along));
    const nmath::scalar_t rx = ex - clamped * dx;
    const nmath::scalar_t ry = ey - clamped * dy;
    return (nmath::scalar_t)std::sqrt((double)(rx * rx + ry * ry));
}

} // namespace

Scratches::Scratches()
    : color_base(nimg::ColorRGBf(0.60f, 0.60f, 0.62f))
    , color_scratch(nimg::ColorRGBf(0.90f, 0.90f, 0.92f))
    , scale((nmath::scalar_t)4.0)
    , density(24)
    , width((nmath::scalar_t)0.008)
    , angle((nmath::scalar_t)0.0)
    , angle_jitter((nmath::scalar_t)0.3)
    , seed(42)
{}

Scratches::~Scratches()
{}

nimg::ColorRGBf Scratches::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)4.0;
    const nmath::scalar_t u = uvw.x * s;
    const nmath::scalar_t v = uvw.y * s;

    const int iu = (int)std::floor((double)u);
    const int iv = (int)std::floor((double)v);

    const nmath::scalar_t w = (width > (nmath::scalar_t)1e-6) ? width * s : (nmath::scalar_t)0.02;
    const int n  = (density < 1) ? 1 : density;
    const nmath::scalar_t pi = (nmath::scalar_t)3.14159265358979323846;

    nmath::scalar_t min_dist = (nmath::scalar_t)1e30;

    // Search current and 8 neighbouring cells for scratch segments
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int cx = iu + dx;
            const int cy = iv + dy;
            for (int k = 0; k < n; ++k) {
                const uint32_t sk = (uint32_t)seed;
                const nmath::scalar_t cx01 = h01(uhash((uint32_t)cx, (uint32_t)cy), (uint32_t)(k * 3 + 0) ^ sk);
                const nmath::scalar_t cy01 = h01(uhash((uint32_t)cx, (uint32_t)cy), (uint32_t)(k * 3 + 1) ^ sk);
                const nmath::scalar_t aj   = h01(uhash((uint32_t)cx, (uint32_t)cy), (uint32_t)(k * 3 + 2) ^ sk);
                const nmath::scalar_t len  = h01(uhash((uint32_t)cx * 7u, (uint32_t)cy * 13u), (uint32_t)k ^ sk);

                const nmath::scalar_t a = angle + angle_jitter * (aj * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0) * pi;
                const nmath::scalar_t cosA = (nmath::scalar_t)std::cos((double)a);
                const nmath::scalar_t sinA = (nmath::scalar_t)std::sin((double)a);

                const nmath::scalar_t scx = (nmath::scalar_t)cx + cx01;
                const nmath::scalar_t scy = (nmath::scalar_t)cy + cy01;
                const nmath::scalar_t half_len = ((nmath::scalar_t)0.2 + (nmath::scalar_t)0.8 * len) * (nmath::scalar_t)0.5;

                const nmath::scalar_t d = seg_dist(u, v, scx, scy, cosA, sinA, half_len);
                if (d < min_dist) min_dist = d;
            }
        }
    }

    const nmath::scalar_t t = clamp01((nmath::scalar_t)1.0 - min_dist / w);
    return color_base * ((nmath::scalar_t)1.0 - t) + color_scratch * t;
}

    } /* namespace sampler */
} /* namespace xtcore */
