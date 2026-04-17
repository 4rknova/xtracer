#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_fbm_wood.h"

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
    h ^= (h >> 13);
    h *= 0x85ebca6bu;
    h ^= (h >> 16);
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

inline nmath::scalar_t fbm(nmath::scalar_t x, nmath::scalar_t y, int octaves, nmath::scalar_t lacunarity, nmath::scalar_t gain)
{
    nmath::scalar_t value = (nmath::scalar_t)0.0;
    nmath::scalar_t amp   = (nmath::scalar_t)0.5;
    nmath::scalar_t freq  = (nmath::scalar_t)1.0;
    for (int i = 0; i < octaves; ++i) {
        value += amp * ((nmath::scalar_t)2.0 * value_noise(x * freq, y * freq) - (nmath::scalar_t)1.0);
        freq  *= lacunarity;
        amp   *= gain;
    }
    return value;
}

} // namespace

FBMWood::FBMWood()
    : color_a(nimg::ColorRGBf(0.72f, 0.48f, 0.22f))
    , color_b(nimg::ColorRGBf(0.36f, 0.20f, 0.08f))
    , scale((nmath::scalar_t)4.0)
    , ring_frequency((nmath::scalar_t)8.0)
    , turbulence((nmath::scalar_t)2.0)
    , octaves(4)
    , lacunarity((nmath::scalar_t)2.0)
    , gain((nmath::scalar_t)0.5)
{}

FBMWood::~FBMWood()
{}

nimg::ColorRGBf FBMWood::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s   = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)4.0;
    const int oct             = (octaves < 1) ? 1 : octaves;
    const nmath::scalar_t lac = (lacunarity < (nmath::scalar_t)1.0) ? (nmath::scalar_t)1.0 : lacunarity;
    const nmath::scalar_t g   = clamp01(gain);

    const nmath::scalar_t x = uvw.x * s;
    const nmath::scalar_t y = uvw.y * s;

    const nmath::scalar_t distortion = turbulence * fbm(x, y, oct, lac, g);
    const nmath::scalar_t dist = std::sqrt((double)(x * x + y * y));
    const nmath::scalar_t ring = dist * ring_frequency + distortion;
    const nmath::scalar_t t    = clamp01((nmath::scalar_t)0.5 + (nmath::scalar_t)0.5 * (nmath::scalar_t)std::sin((double)ring));

    return color_a * ((nmath::scalar_t)1.0 - t) + color_b * t;
}

    } /* namespace sampler */
} /* namespace xtcore */
