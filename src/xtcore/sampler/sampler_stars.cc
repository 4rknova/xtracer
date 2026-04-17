#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_stars.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline uint32_t uhash3(int x, int y, int seed)
{
    uint32_t h = (uint32_t)seed ^ ((uint32_t)x * 0x9e3779b9u) ^ ((uint32_t)y * 0x85ebca6bu);
    h ^= (h >> 16); h *= 0x45d9f3bu; h ^= (h >> 16);
    return h;
}

inline nmath::scalar_t h01(int x, int y, int seed, uint32_t ch)
{
    const uint32_t h = uhash3(x, y, seed) ^ (ch * 0xc2b2ae35u);
    return (nmath::scalar_t)(h & 0x00ffffffu) / (nmath::scalar_t)0x01000000u;
}

} // namespace

Stars::Stars()
    : background_color(nimg::ColorRGBf(0.0f, 0.0f, 0.02f))
    , density((nmath::scalar_t)0.015)
    , min_brightness((nmath::scalar_t)0.4)
    , max_brightness((nmath::scalar_t)1.0)
    , star_size((nmath::scalar_t)0.015)
    , seed(42)
{}

Stars::~Stars()
{}

nimg::ColorRGBf Stars::sample(const nmath::Vector3f &uvw) const
{
    // Convert direction to spherical UV for sampling
    const nmath::scalar_t len = (nmath::scalar_t)std::sqrt((double)(uvw.x*uvw.x + uvw.y*uvw.y + uvw.z*uvw.z));
    nmath::scalar_t u, v;
    if (len < (nmath::scalar_t)1e-6) {
        u = uvw.x;
        v = uvw.y;
    } else {
        const nmath::scalar_t pi = (nmath::scalar_t)3.14159265358979323846;
        u = ((nmath::scalar_t)std::atan2((double)(uvw.x / len), (double)(uvw.z / len)) / ((nmath::scalar_t)2.0 * pi) + (nmath::scalar_t)0.5);
        v = ((nmath::scalar_t)std::acos((double)std::max((nmath::scalar_t)-1.0, std::min((nmath::scalar_t)1.0, uvw.y / len))) / pi);
    }

    // Grid density: higher = more tiles = more stars per solid angle
    const nmath::scalar_t grid_scale = (nmath::scalar_t)256.0;
    const nmath::scalar_t gu = u * grid_scale;
    const nmath::scalar_t gv = v * grid_scale;
    const int iu = (int)std::floor((double)gu);
    const int iv = (int)std::floor((double)gv);

    const nmath::scalar_t fu = gu - (nmath::scalar_t)iu;
    const nmath::scalar_t fv = gv - (nmath::scalar_t)iv;

    nimg::ColorRGBf result = background_color;

    // Check current cell and 8 neighbours for a star centre
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int cx = iu + dx;
            const int cy = iv + dy;
            // Only spawn a star if the cell's random value is below density
            const nmath::scalar_t presence = h01(cx, cy, seed, 0u);
            if (presence > density) continue;

            // Star position within cell
            const nmath::scalar_t sx = (nmath::scalar_t)dx + h01(cx, cy, seed, 1u);
            const nmath::scalar_t sy = (nmath::scalar_t)dy + h01(cx, cy, seed, 2u);
            const nmath::scalar_t dist = (nmath::scalar_t)std::sqrt((double)((fu - sx)*(fu - sx) + (fv - sy)*(fv - sy)));

            const nmath::scalar_t sz   = star_size + (nmath::scalar_t)1e-6;
            const nmath::scalar_t gaus = (nmath::scalar_t)std::exp((double)(-(dist*dist) / ((nmath::scalar_t)2.0 * sz*sz)));

            const nmath::scalar_t brightness = min_brightness + (max_brightness - min_brightness) * h01(cx, cy, seed, 3u);
            // Slight colour temperature variation: hot = blue-white, cool = orange-white
            const nmath::scalar_t temp  = h01(cx, cy, seed, 4u);
            const nimg::ColorRGBf sc(
                clamp01(brightness * ((nmath::scalar_t)0.8 + (nmath::scalar_t)0.2 * ((nmath::scalar_t)1.0 - temp))),
                clamp01(brightness * ((nmath::scalar_t)0.85 + (nmath::scalar_t)0.1 * ((nmath::scalar_t)1.0 - temp))),
                clamp01(brightness * ((nmath::scalar_t)0.9  + (nmath::scalar_t)0.1 * temp))
            );

            result = result + sc * gaus;
        }
    }

    return result;
}

    } /* namespace sampler */
} /* namespace xtcore */
