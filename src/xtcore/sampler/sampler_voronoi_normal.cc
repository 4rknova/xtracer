#include <algorithm>
#include <cmath>
#include <cstdint>

#include "sampler_voronoi_normal.h"

namespace xtcore {
    namespace sampler {

namespace {

inline int positive_mod(int x, int m)
{
    const int r = x % m;
    return (r < 0) ? (r + m) : r;
}

inline nmath::scalar_t fract(nmath::scalar_t v)
{
    return v - (nmath::scalar_t)std::floor((double)v);
}

inline uint32_t hash_u32(int x, int y, int seed, uint32_t channel)
{
    uint32_t h = (uint32_t)seed;
    h ^= (uint32_t)x * 0x9e3779b9u;
    h ^= (uint32_t)y * 0x85ebca6bu;
    h ^= channel * 0xc2b2ae35u;
    h ^= (h >> 16);
    h *= 0x7feb352du;
    h ^= (h >> 15);
    h *= 0x846ca68bu;
    h ^= (h >> 16);
    return h;
}

inline nmath::scalar_t hash01(int x, int y, int seed, uint32_t channel)
{
    const uint32_t h = hash_u32(x, y, seed, channel);
    return (nmath::scalar_t)(h & 0x00ffffffu) / (nmath::scalar_t)0x01000000u;
}

} // namespace

VoronoiNormal::VoronoiNormal()
    : cells(64)
    , max_deviation((nmath::scalar_t)12.0)
    , seed(1337)
{}

VoronoiNormal::~VoronoiNormal()
{}

nimg::ColorRGBf VoronoiNormal::sample(const nmath::Vector3f &uvw) const
{
    const int cell_count = (cells < 1) ? 1 : cells;
    const int grid = std::max(1, (int)std::ceil(std::sqrt((double)cell_count)));

    const nmath::scalar_t u = fract(uvw.x);
    const nmath::scalar_t v = fract(uvw.y);

    const nmath::scalar_t gx = u * (nmath::scalar_t)grid;
    const nmath::scalar_t gy = v * (nmath::scalar_t)grid;
    const int ix = (int)std::floor((double)gx);
    const int iy = (int)std::floor((double)gy);

    nmath::scalar_t best_d2 = (nmath::scalar_t)1e30;
    int best_x = 0;
    int best_y = 0;

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int cx = ix + dx;
            const int cy = iy + dy;
            const int wx = positive_mod(cx, grid);
            const int wy = positive_mod(cy, grid);

            const nmath::scalar_t jx = hash01(wx, wy, seed, 0u);
            const nmath::scalar_t jy = hash01(wx, wy, seed, 1u);
            const nmath::scalar_t sx = (nmath::scalar_t)cx + jx;
            const nmath::scalar_t sy = (nmath::scalar_t)cy + jy;
            const nmath::scalar_t dxp = gx - sx;
            const nmath::scalar_t dyp = gy - sy;
            const nmath::scalar_t d2 = dxp * dxp + dyp * dyp;

            if (d2 < best_d2) {
                best_d2 = d2;
                best_x = wx;
                best_y = wy;
            }
        }
    }

    nmath::scalar_t max_dev = max_deviation;
    if (max_dev < (nmath::scalar_t)0.0) max_dev = (nmath::scalar_t)0.0;
    if (max_dev > (nmath::scalar_t)89.0) max_dev = (nmath::scalar_t)89.0;

    const nmath::scalar_t pi = (nmath::scalar_t)3.14159265358979323846;
    const nmath::scalar_t theta = hash01(best_x, best_y, seed, 2u) * (max_dev * (pi / (nmath::scalar_t)180.0));
    const nmath::scalar_t phi = hash01(best_x, best_y, seed, 3u) * ((nmath::scalar_t)2.0 * pi);

    const nmath::scalar_t st = (nmath::scalar_t)std::sin((double)theta);
    nmath::Vector3f n(
        st * (nmath::scalar_t)std::cos((double)phi),
        st * (nmath::scalar_t)std::sin((double)phi),
        (nmath::scalar_t)std::cos((double)theta)
    );
    n.normalize();

    return nimg::ColorRGBf(
        (n.x + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5,
        (n.y + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5,
        (n.z + (nmath::scalar_t)1.0) * (nmath::scalar_t)0.5
    );
}

    } /* namespace sampler */
} /* namespace xtcore */
