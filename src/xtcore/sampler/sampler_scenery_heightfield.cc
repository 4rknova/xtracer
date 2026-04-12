#include <algorithm>
#include <cmath>
#include <cstdint>

#include "sampler_scenery_heightfield.h"

namespace xtcore {
    namespace sampler {

namespace {

inline nmath::scalar_t clamp01(nmath::scalar_t v)
{
    return std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, v));
}

inline nmath::scalar_t lerp(nmath::scalar_t a, nmath::scalar_t b, nmath::scalar_t t)
{
    return a + (b - a) * t;
}

inline nmath::scalar_t smoothstep(nmath::scalar_t t)
{
    t = clamp01(t);
    return t * t * ((nmath::scalar_t)3.0 - (nmath::scalar_t)2.0 * t);
}

inline nmath::scalar_t fractf(nmath::scalar_t x)
{
    return x - (nmath::scalar_t)std::floor((double)x);
}

inline nmath::scalar_t hash2(int x, int y, uint32_t seed)
{
    uint32_t h = (uint32_t)(x) * 0x8da6b343u ^ (uint32_t)(y) * 0xd8163841u ^ seed * 0x9e3779b9u;
    h ^= (h >> 13);
    h *= 0x85ebca6bu;
    h ^= (h >> 16);
    return (nmath::scalar_t)(h & 0x00ffffffu) / (nmath::scalar_t)0x01000000u;
}

inline nmath::scalar_t value_noise(nmath::scalar_t x, nmath::scalar_t y, uint32_t seed)
{
    const int ix = (int)std::floor((double)x);
    const int iy = (int)std::floor((double)y);
    const nmath::scalar_t fx = x - (nmath::scalar_t)ix;
    const nmath::scalar_t fy = y - (nmath::scalar_t)iy;
    const nmath::scalar_t sx = smoothstep(fx);
    const nmath::scalar_t sy = smoothstep(fy);

    const nmath::scalar_t a = hash2(ix, iy, seed);
    const nmath::scalar_t b = hash2(ix + 1, iy, seed);
    const nmath::scalar_t c = hash2(ix, iy + 1, seed);
    const nmath::scalar_t d = hash2(ix + 1, iy + 1, seed);

    const nmath::scalar_t ab = lerp(a, b, sx);
    const nmath::scalar_t cd = lerp(c, d, sx);
    return lerp(ab, cd, sy) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
}

inline nmath::scalar_t fbm(nmath::scalar_t x, nmath::scalar_t y, uint32_t seed, int octaves, nmath::scalar_t lacunarity, nmath::scalar_t gain)
{
    nmath::scalar_t sum = (nmath::scalar_t)0.0;
    nmath::scalar_t amp = (nmath::scalar_t)1.0;
    nmath::scalar_t freq = (nmath::scalar_t)1.0;
    nmath::scalar_t norm = (nmath::scalar_t)0.0;

    for (int i = 0; i < octaves; ++i) {
        sum += amp * value_noise(x * freq + (nmath::scalar_t)i * 11.7, y * freq + (nmath::scalar_t)i * 7.3, seed + (uint32_t)i * 1664525u);
        norm += amp;
        amp *= gain;
        freq *= lacunarity;
    }

    if (norm <= (nmath::scalar_t)1e-8) return 0.0;
    return sum / norm;
}

inline nmath::scalar_t ridged(nmath::scalar_t x, nmath::scalar_t y, uint32_t seed, int octaves, nmath::scalar_t lacunarity, nmath::scalar_t gain)
{
    nmath::scalar_t sum = (nmath::scalar_t)0.0;
    nmath::scalar_t amp = (nmath::scalar_t)0.5;
    nmath::scalar_t freq = (nmath::scalar_t)1.0;
    nmath::scalar_t norm = (nmath::scalar_t)0.0;

    for (int i = 0; i < octaves; ++i) {
        const nmath::scalar_t n = value_noise(x * freq + (nmath::scalar_t)i * 5.1, y * freq + (nmath::scalar_t)i * 9.2, seed + 911382323u + (uint32_t)i * 2654435761u);
        const nmath::scalar_t r = (nmath::scalar_t)1.0 - std::fabs((double)n);
        sum += amp * r * r;
        norm += amp;
        amp *= gain;
        freq *= lacunarity;
    }

    if (norm <= (nmath::scalar_t)1e-8) return 0.0;
    return sum / norm;
}

} // namespace

SceneryHeightfield::SceneryHeightfield()
    : seed(1337)
    , scale((nmath::scalar_t)0.25)
    , octaves(5)
    , lacunarity((nmath::scalar_t)2.0)
    , gain((nmath::scalar_t)0.5)
    , ridge_strength((nmath::scalar_t)0.65)
    , mountain_strength((nmath::scalar_t)0.75)
    , valley_strength((nmath::scalar_t)0.55)
{}

SceneryHeightfield::~SceneryHeightfield()
{}

nimg::ColorRGBf SceneryHeightfield::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)0.25;
    const int oct = std::max(1, octaves);
    const nmath::scalar_t lac = std::max((nmath::scalar_t)1.01, lacunarity);
    const nmath::scalar_t g = std::max((nmath::scalar_t)0.05, std::min((nmath::scalar_t)0.95, gain));

    const nmath::scalar_t x = uvw.x * s;
    const nmath::scalar_t y = uvw.z * s;

    const nmath::scalar_t base = fbm(x * (nmath::scalar_t)0.35, y * (nmath::scalar_t)0.35, (uint32_t)seed, oct, (nmath::scalar_t)1.9, (nmath::scalar_t)0.55);
    const nmath::scalar_t ridges = ridged(x, y, (uint32_t)seed ^ 0x9e3779b9u, std::max(2, oct - 1), lac, g);

    const nmath::scalar_t basin = base * (nmath::scalar_t)0.5 + (nmath::scalar_t)0.5;
    const nmath::scalar_t continent = basin * basin;
    const nmath::scalar_t mountains = std::max((nmath::scalar_t)0.0, base) * ridges;
    const nmath::scalar_t valleys = std::pow(std::max((nmath::scalar_t)0.0, (nmath::scalar_t)1.0 - basin), (nmath::scalar_t)1.5);

    nmath::scalar_t h = continent * valley_strength + mountains * ridge_strength * mountain_strength - valleys * ((nmath::scalar_t)0.35 * valley_strength);
    h = h * (nmath::scalar_t)0.5 + (nmath::scalar_t)0.5;
    h = clamp01(h);

    return nimg::ColorRGBf((float)h, (float)h, (float)h);
}

    } /* namespace sampler */
} /* namespace xtcore */
