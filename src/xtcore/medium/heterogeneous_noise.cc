#include <algorithm>
#include <cmath>
#include <new>

#include <nimg/luminance.h>
#include <nmath/prng.h>

#include "heterogeneous_noise.h"

namespace xtcore {
namespace asset {
namespace medium {

namespace {

inline uint32_t hash_u32(int x, int y, int z, int seed, uint32_t channel)
{
    uint32_t h = static_cast<uint32_t>(seed);
    h ^= static_cast<uint32_t>(x) * 374761393u;
    h ^= static_cast<uint32_t>(y) * 668265263u;
    h ^= static_cast<uint32_t>(z) * 2147483647u;
    h ^= channel * 1274126177u;
    h ^= h >> 13;
    h *= 1274126177u;
    h ^= h >> 16;
    return h;
}

inline nmath::scalar_t hash01(int x, int y, int z, int seed, uint32_t channel)
{
    const uint32_t h = hash_u32(x, y, z, seed, channel);
    return static_cast<nmath::scalar_t>((h & 0x00ffffffu) / 16777215.0);
}

inline nmath::scalar_t lerp_scalar(nmath::scalar_t a, nmath::scalar_t b, nmath::scalar_t t)
{
    return a + (b - a) * t;
}

inline nmath::scalar_t smoothstep_perlin(nmath::scalar_t x)
{
    const nmath::scalar_t t = std::max((nmath::scalar_t)0.0, std::min((nmath::scalar_t)1.0, x));
    return t * t * t * (t * (t * (nmath::scalar_t)6.0 - (nmath::scalar_t)15.0) + (nmath::scalar_t)10.0);
}

inline nmath::Vector3f gradient_dir(int x, int y, int z, int seed, uint32_t channel)
{
    const nmath::scalar_t gx = hash01(x, y, z, seed, channel + 0u) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
    const nmath::scalar_t gy = hash01(x, y, z, seed, channel + 1u) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
    const nmath::scalar_t gz = hash01(x, y, z, seed, channel + 2u) * (nmath::scalar_t)2.0 - (nmath::scalar_t)1.0;
    nmath::Vector3f g(gx, gy, gz);
    if (g.length_squared() <= (nmath::scalar_t)EPSILON) return nmath::Vector3f((nmath::scalar_t)1.0, (nmath::scalar_t)0.0, (nmath::scalar_t)0.0);
    return g.normalized();
}

inline nmath::scalar_t perlin_noise_3d(nmath::scalar_t x, nmath::scalar_t y, nmath::scalar_t z, int seed)
{
    const int ix = static_cast<int>(std::floor((double)x));
    const int iy = static_cast<int>(std::floor((double)y));
    const int iz = static_cast<int>(std::floor((double)z));

    const nmath::scalar_t fx = x - static_cast<nmath::scalar_t>(ix);
    const nmath::scalar_t fy = y - static_cast<nmath::scalar_t>(iy);
    const nmath::scalar_t fz = z - static_cast<nmath::scalar_t>(iz);

    const nmath::scalar_t ux = smoothstep_perlin(fx);
    const nmath::scalar_t uy = smoothstep_perlin(fy);
    const nmath::scalar_t uz = smoothstep_perlin(fz);

    const nmath::Vector3f g000 = gradient_dir(ix + 0, iy + 0, iz + 0, seed, 0u);
    const nmath::Vector3f g100 = gradient_dir(ix + 1, iy + 0, iz + 0, seed, 3u);
    const nmath::Vector3f g010 = gradient_dir(ix + 0, iy + 1, iz + 0, seed, 6u);
    const nmath::Vector3f g110 = gradient_dir(ix + 1, iy + 1, iz + 0, seed, 9u);
    const nmath::Vector3f g001 = gradient_dir(ix + 0, iy + 0, iz + 1, seed, 12u);
    const nmath::Vector3f g101 = gradient_dir(ix + 1, iy + 0, iz + 1, seed, 15u);
    const nmath::Vector3f g011 = gradient_dir(ix + 0, iy + 1, iz + 1, seed, 18u);
    const nmath::Vector3f g111 = gradient_dir(ix + 1, iy + 1, iz + 1, seed, 21u);

    const nmath::scalar_t c000 = nmath::dot(g000, nmath::Vector3f(fx - (nmath::scalar_t)0.0, fy - (nmath::scalar_t)0.0, fz - (nmath::scalar_t)0.0));
    const nmath::scalar_t c100 = nmath::dot(g100, nmath::Vector3f(fx - (nmath::scalar_t)1.0, fy - (nmath::scalar_t)0.0, fz - (nmath::scalar_t)0.0));
    const nmath::scalar_t c010 = nmath::dot(g010, nmath::Vector3f(fx - (nmath::scalar_t)0.0, fy - (nmath::scalar_t)1.0, fz - (nmath::scalar_t)0.0));
    const nmath::scalar_t c110 = nmath::dot(g110, nmath::Vector3f(fx - (nmath::scalar_t)1.0, fy - (nmath::scalar_t)1.0, fz - (nmath::scalar_t)0.0));
    const nmath::scalar_t c001 = nmath::dot(g001, nmath::Vector3f(fx - (nmath::scalar_t)0.0, fy - (nmath::scalar_t)0.0, fz - (nmath::scalar_t)1.0));
    const nmath::scalar_t c101 = nmath::dot(g101, nmath::Vector3f(fx - (nmath::scalar_t)1.0, fy - (nmath::scalar_t)0.0, fz - (nmath::scalar_t)1.0));
    const nmath::scalar_t c011 = nmath::dot(g011, nmath::Vector3f(fx - (nmath::scalar_t)0.0, fy - (nmath::scalar_t)1.0, fz - (nmath::scalar_t)1.0));
    const nmath::scalar_t c111 = nmath::dot(g111, nmath::Vector3f(fx - (nmath::scalar_t)1.0, fy - (nmath::scalar_t)1.0, fz - (nmath::scalar_t)1.0));

    const nmath::scalar_t x00 = lerp_scalar(c000, c100, ux);
    const nmath::scalar_t x10 = lerp_scalar(c010, c110, ux);
    const nmath::scalar_t x01 = lerp_scalar(c001, c101, ux);
    const nmath::scalar_t x11 = lerp_scalar(c011, c111, ux);

    const nmath::scalar_t y0 = lerp_scalar(x00, x10, uy);
    const nmath::scalar_t y1 = lerp_scalar(x01, x11, uy);

    // Normalize from approximately [-1,1] to [0,1].
    return lerp_scalar(y0, y1, uz) * (nmath::scalar_t)0.5 + (nmath::scalar_t)0.5;
}

inline nmath::scalar_t fbm_3d(nmath::scalar_t x,
                              nmath::scalar_t y,
                              nmath::scalar_t z,
                              int octaves,
                              nmath::scalar_t lacunarity,
                              nmath::scalar_t gain,
                              int seed)
{
    int oct = std::max(1, std::min(12, octaves));
    nmath::scalar_t freq = (nmath::scalar_t)1.0;
    nmath::scalar_t amp = (nmath::scalar_t)1.0;
    nmath::scalar_t sum = (nmath::scalar_t)0.0;
    nmath::scalar_t norm = (nmath::scalar_t)0.0;

    for (int i = 0; i < oct; ++i) {
        sum += amp * perlin_noise_3d(x * freq, y * freq, z * freq, seed + i * 131);
        norm += amp;
        freq *= std::max((nmath::scalar_t)1.0, lacunarity);
        amp *= std::max((nmath::scalar_t)0.05, std::min((nmath::scalar_t)0.95, gain));
    }

    if (norm <= (nmath::scalar_t)EPSILON) return (nmath::scalar_t)0.0;
    return sum / norm;
}

inline nmath::scalar_t clamp_scalar(nmath::scalar_t v, nmath::scalar_t lo, nmath::scalar_t hi)
{
    return std::max(lo, std::min(hi, v));
}

} // namespace

HeterogeneousNoise::HeterogeneousNoise()
    : m_sigma_a(0.0f, 0.0f, 0.0f)
    , m_sigma_s(0.0f, 0.0f, 0.0f)
    , m_emission(0.0f, 0.0f, 0.0f)
    , m_g(0.0f)
    , m_density_multiplier(1.0f)
    , m_noise_scale(1.0f)
    , m_noise_min(0.25f)
    , m_noise_max(1.0f)
    , m_octaves(4)
    , m_lacunarity(2.0f)
    , m_gain(0.5f)
    , m_seed(1337)
{}

HeterogeneousNoise::HeterogeneousNoise(
      const nimg::ColorRGBf &sigma_a
    , const nimg::ColorRGBf &sigma_s
    , const nimg::ColorRGBf &emission
    , nmath::scalar_t g
    , nmath::scalar_t density_multiplier
    , nmath::scalar_t noise_scale
    , nmath::scalar_t noise_min
    , nmath::scalar_t noise_max
    , int octaves
    , nmath::scalar_t lacunarity
    , nmath::scalar_t gain
    , int seed
)
    : m_sigma_a(sigma_a)
    , m_sigma_s(sigma_s)
    , m_emission(emission)
    , m_g(g)
    , m_density_multiplier(std::max((nmath::scalar_t)0.0, density_multiplier))
    , m_noise_scale(std::max((nmath::scalar_t)1e-4, noise_scale))
    , m_noise_min(noise_min)
    , m_noise_max(noise_max)
    , m_octaves(std::max(1, std::min(12, octaves)))
    , m_lacunarity(std::max((nmath::scalar_t)1.0, lacunarity))
    , m_gain(std::max((nmath::scalar_t)0.05, std::min((nmath::scalar_t)0.95, gain)))
    , m_seed(seed)
{
    if (m_noise_min > m_noise_max) std::swap(m_noise_min, m_noise_max);
    m_noise_min = std::max((nmath::scalar_t)0.0, m_noise_min);
    m_noise_max = std::max((nmath::scalar_t)0.0, m_noise_max);
}

HeterogeneousNoise::~HeterogeneousNoise()
{}

IMedium *HeterogeneousNoise::clone() const
{
    return new (std::nothrow) HeterogeneousNoise(
          m_sigma_a
        , m_sigma_s
        , m_emission
        , m_g
        , m_density_multiplier
        , m_noise_scale
        , m_noise_min
        , m_noise_max
        , m_octaves
        , m_lacunarity
        , m_gain
        , m_seed
    );
}

nmath::scalar_t HeterogeneousNoise::density_at(const nmath::Vector3f &p) const
{
    const nmath::scalar_t n = fbm_3d(
          p.x * m_noise_scale
        , p.y * m_noise_scale
        , p.z * m_noise_scale
        , m_octaves
        , m_lacunarity
        , m_gain
        , m_seed
    );
    const nmath::scalar_t f = clamp_scalar(n, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
    const nmath::scalar_t d = lerp_scalar(m_noise_min, m_noise_max, f);
    return std::max((nmath::scalar_t)0.0, d * m_density_multiplier);
}

nimg::ColorRGBf HeterogeneousNoise::sigma_a() const
{
    const nmath::scalar_t d = m_density_multiplier * ((m_noise_min + m_noise_max) * (nmath::scalar_t)0.5);
    return m_sigma_a * std::max((nmath::scalar_t)0.0, d);
}

nimg::ColorRGBf HeterogeneousNoise::sigma_s() const
{
    const nmath::scalar_t d = m_density_multiplier * ((m_noise_min + m_noise_max) * (nmath::scalar_t)0.5);
    return m_sigma_s * std::max((nmath::scalar_t)0.0, d);
}

nimg::ColorRGBf HeterogeneousNoise::emission() const
{
    const nmath::scalar_t d = m_density_multiplier * ((m_noise_min + m_noise_max) * (nmath::scalar_t)0.5);
    return m_emission * std::max((nmath::scalar_t)0.0, d);
}

nmath::scalar_t HeterogeneousNoise::asymmetry() const
{
    return m_g;
}

nimg::ColorRGBf HeterogeneousNoise::sigma_a_at(const nmath::Vector3f &p) const
{
    return m_sigma_a * density_at(p);
}

nimg::ColorRGBf HeterogeneousNoise::sigma_s_at(const nmath::Vector3f &p) const
{
    return m_sigma_s * density_at(p);
}

nimg::ColorRGBf HeterogeneousNoise::emission_at(const nmath::Vector3f &p) const
{
    return m_emission * density_at(p);
}

nmath::scalar_t HeterogeneousNoise::sigma_t_majorant() const
{
    const nimg::ColorRGBf st = (m_sigma_a + m_sigma_s) * (m_density_multiplier * std::max(m_noise_min, m_noise_max));
    return std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(st));
}

nimg::ColorRGBf HeterogeneousNoise::transmittance(const nmath::Vector3f &origin,
                                                  const nmath::Vector3f &direction,
                                                  nmath::scalar_t dist) const
{
    const nmath::scalar_t d = std::max((nmath::scalar_t)0.0, dist);
    if (d <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(1.0f, 1.0f, 1.0f);

    const nmath::Vector3f dir = (direction.length_squared() > (nmath::scalar_t)EPSILON)
        ? direction.normalized()
        : nmath::Vector3f(0.0f, 0.0f, 1.0f);

    const int steps = std::max(12, std::min(192, static_cast<int>(d * (nmath::scalar_t)24.0) + 12));
    const nmath::scalar_t dt = d / static_cast<nmath::scalar_t>(steps);

    nmath::scalar_t tr_r = (nmath::scalar_t)1.0;
    nmath::scalar_t tr_g = (nmath::scalar_t)1.0;
    nmath::scalar_t tr_b = (nmath::scalar_t)1.0;

    for (int i = 0; i < steps; ++i) {
        const nmath::scalar_t t = (static_cast<nmath::scalar_t>(i) + (nmath::scalar_t)0.5) * dt;
        const nmath::Vector3f p = origin + dir * t;
        const nimg::ColorRGBf st = sigma_t_at(p);
        tr_r *= (nmath::scalar_t)std::exp(-(double)(st.r() * dt));
        tr_g *= (nmath::scalar_t)std::exp(-(double)(st.g() * dt));
        tr_b *= (nmath::scalar_t)std::exp(-(double)(st.b() * dt));
    }

    return nimg::ColorRGBf(tr_r, tr_g, tr_b);
}

bool HeterogeneousNoise::sample_distance(const nmath::Vector3f &origin,
                                         const nmath::Vector3f &direction,
                                         nmath::scalar_t segment_dist,
                                         nmath::scalar_t &sampled_dist) const
{
    const nmath::scalar_t max_t = std::max((nmath::scalar_t)0.0, segment_dist);
    const nmath::scalar_t majorant = sigma_t_majorant();
    if (majorant <= (nmath::scalar_t)EPSILON || max_t <= (nmath::scalar_t)EPSILON) {
        sampled_dist = max_t;
        return false;
    }

    const nmath::Vector3f dir = (direction.length_squared() > (nmath::scalar_t)EPSILON)
        ? direction.normalized()
        : nmath::Vector3f(0.0f, 0.0f, 1.0f);

    nmath::scalar_t t = (nmath::scalar_t)0.0;
    for (int iter = 0; iter < 1024; ++iter) {
        const nmath::scalar_t u = std::max(
            (nmath::scalar_t)1e-6,
            std::min((nmath::scalar_t)0.999999, nmath::prng_c(0.0, 1.0))
        );
        t += -(nmath::scalar_t)std::log((double)(1.0 - u)) / majorant;

        if (t >= max_t) {
            sampled_dist = max_t;
            return false;
        }

        const nmath::Vector3f p = origin + dir * t;
        const nmath::scalar_t local_st = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(sigma_t_at(p)));
        const nmath::scalar_t accept = clamp_scalar(local_st / majorant, (nmath::scalar_t)0.0, (nmath::scalar_t)1.0);
        if (nmath::prng_c(0.0, 1.0) < accept) {
            sampled_dist = t;
            return true;
        }
    }

    sampled_dist = max_t;
    return false;
}

nimg::ColorRGBf HeterogeneousNoise::scattering_weight(const nmath::Vector3f &p) const
{
    const nmath::scalar_t st = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(sigma_t_at(p)));
    if (st <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
    return sigma_s_at(p) * ((nmath::scalar_t)1.0 / st);
}

} // namespace medium
} // namespace asset
} // namespace xtcore
