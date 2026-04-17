#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_curl_noise.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline nmath::scalar_t smoothstep(nmath::scalar_t t)
{
    return t * t * ((nmath::scalar_t)3.0 - (nmath::scalar_t)2.0 * t);
}

inline nmath::scalar_t hash2(int x, int y)
{
    uint32_t h = (uint32_t)(x) * 0x8da6b343u ^ (uint32_t)(y) * 0xd8163841u;
    h ^= (h >> 13); h *= 0x85ebca6bu; h ^= (h >> 16);
    return (nmath::scalar_t)(h & 0x00ffffffu) / (nmath::scalar_t)0x01000000u;
}

inline nmath::scalar_t value_noise(nmath::scalar_t x, nmath::scalar_t y)
{
    const int ix = (int)std::floor((double)x);
    const int iy = (int)std::floor((double)y);
    const nmath::scalar_t fx = x - (nmath::scalar_t)ix;
    const nmath::scalar_t fy = y - (nmath::scalar_t)iy;
    const nmath::scalar_t sx = smoothstep(fx);
    const nmath::scalar_t sy = smoothstep(fy);
    const nmath::scalar_t a = hash2(ix,     iy);
    const nmath::scalar_t b = hash2(ix + 1, iy);
    const nmath::scalar_t c = hash2(ix,     iy + 1);
    const nmath::scalar_t d = hash2(ix + 1, iy + 1);
    return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
}

inline nmath::scalar_t fbm_val(nmath::scalar_t x, nmath::scalar_t y, int oct, nmath::scalar_t lac, nmath::scalar_t g)
{
    nmath::scalar_t v = (nmath::scalar_t)0.0, amp = (nmath::scalar_t)0.5, freq = (nmath::scalar_t)1.0;
    for (int i = 0; i < oct; ++i) {
        v += amp * ((nmath::scalar_t)2.0 * value_noise(x * freq, y * freq) - (nmath::scalar_t)1.0);
        freq *= lac; amp *= g;
    }
    return v;
}

// Numerical curl: rotate gradient of a scalar noise field 90 degrees.
// This gives a divergence-free 2D flow field without swirling artifacts.
static const nmath::scalar_t EPS = (nmath::scalar_t)1e-3;

inline void curl(nmath::scalar_t x, nmath::scalar_t y, int oct, nmath::scalar_t lac, nmath::scalar_t g,
                 nmath::scalar_t &cx, nmath::scalar_t &cy)
{
    const nmath::scalar_t dydx = (fbm_val(x + EPS, y,       oct, lac, g) - fbm_val(x - EPS, y,       oct, lac, g)) / ((nmath::scalar_t)2.0 * EPS);
    const nmath::scalar_t dydy = (fbm_val(x,       y + EPS, oct, lac, g) - fbm_val(x,       y - EPS, oct, lac, g)) / ((nmath::scalar_t)2.0 * EPS);
    cx =  dydy;
    cy = -dydx;
}

} // namespace

CurlNoise::CurlNoise()
    : color_a(nimg::ColorRGBf(0.10f, 0.18f, 0.42f))
    , color_b(nimg::ColorRGBf(0.82f, 0.90f, 0.98f))
    , scale((nmath::scalar_t)3.0)
    , strength((nmath::scalar_t)1.0)
    , octaves(4)
    , lacunarity((nmath::scalar_t)2.0)
    , gain((nmath::scalar_t)0.5)
{}

CurlNoise::~CurlNoise()
{}

nimg::ColorRGBf CurlNoise::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s   = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)3.0;
    const int oct             = (octaves < 1) ? 1 : octaves;
    const nmath::scalar_t lac = (lacunarity < (nmath::scalar_t)1.0) ? (nmath::scalar_t)1.0 : lacunarity;
    const nmath::scalar_t g   = clamp01(gain);

    const nmath::scalar_t x = uvw.x * s;
    const nmath::scalar_t y = uvw.y * s;

    nmath::scalar_t cx, cy;
    curl(x, y, oct, lac, g, cx, cy);

    const nmath::scalar_t wx = x + strength * cx;
    const nmath::scalar_t wy = y + strength * cy;

    const nmath::scalar_t n = clamp01((nmath::scalar_t)0.5 + (nmath::scalar_t)0.5 * fbm_val(wx, wy, oct, lac, g));

    return color_a * ((nmath::scalar_t)1.0 - n) + color_b * n;
}

    } /* namespace sampler */
} /* namespace xtcore */
