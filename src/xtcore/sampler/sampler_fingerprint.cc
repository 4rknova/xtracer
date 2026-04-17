#include <cmath>
#include <cstdint>
#include <algorithm>

#include "sampler_fingerprint.h"

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

} // namespace

Fingerprint::Fingerprint()
    : color_a(nimg::ColorRGBf(0.18f, 0.16f, 0.14f))
    , color_b(nimg::ColorRGBf(0.50f, 0.46f, 0.42f))
    , scale((nmath::scalar_t)6.0)
    , ridge_frequency((nmath::scalar_t)18.0)
    , ridge_width((nmath::scalar_t)0.4)
    , distortion((nmath::scalar_t)1.8)
{}

Fingerprint::~Fingerprint()
{}

nimg::ColorRGBf Fingerprint::sample(const nmath::Vector3f &uvw) const
{
    const nmath::scalar_t s   = (std::fabs((double)scale) > 1e-6) ? scale : (nmath::scalar_t)6.0;
    const nmath::scalar_t pi  = (nmath::scalar_t)3.14159265358979323846;

    const nmath::scalar_t u = uvw.x * s;
    const nmath::scalar_t v = uvw.y * s;

    // Low-frequency noise for overall arch/loop deformation
    const nmath::scalar_t warp = distortion * ((nmath::scalar_t)2.0 * value_noise(u * (nmath::scalar_t)0.4, v * (nmath::scalar_t)0.4) - (nmath::scalar_t)1.0);
    const nmath::scalar_t angle = (nmath::scalar_t)std::atan2((double)v, (double)(u + (nmath::scalar_t)1e-6)) + warp;

    // Concentric ridge bands radiating from origin, deformed by warp angle
    const nmath::scalar_t dist = (nmath::scalar_t)std::sqrt((double)(u * u + v * v));
    const nmath::scalar_t phase = dist * ridge_frequency + angle * (nmath::scalar_t)2.0;
    const nmath::scalar_t ridge = (nmath::scalar_t)0.5 + (nmath::scalar_t)0.5 * (nmath::scalar_t)std::sin((double)(phase * (nmath::scalar_t)2.0 * pi));

    const nmath::scalar_t w = clamp01(ridge_width);
    const nmath::scalar_t t = clamp01((ridge - ((nmath::scalar_t)1.0 - w)) / (w + (nmath::scalar_t)1e-4));

    return color_a * ((nmath::scalar_t)1.0 - t) + color_b * t;
}

    } /* namespace sampler */
} /* namespace xtcore */
