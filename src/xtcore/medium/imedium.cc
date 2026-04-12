#include <algorithm>
#include <cmath>

#include <nmath/prng.h>
#include <nimg/luminance.h>

#include "imedium.h"

namespace xtcore {
namespace asset {
namespace medium {

IMedium::IMedium()
{}

IMedium::~IMedium()
{}

nimg::ColorRGBf IMedium::sigma_a_at(const nmath::Vector3f &) const
{
    return sigma_a();
}

nimg::ColorRGBf IMedium::sigma_s_at(const nmath::Vector3f &) const
{
    return sigma_s();
}

nimg::ColorRGBf IMedium::emission_at(const nmath::Vector3f &) const
{
    return emission();
}

nimg::ColorRGBf IMedium::sigma_t_at(const nmath::Vector3f &p) const
{
    return sigma_a_at(p) + sigma_s_at(p);
}

nimg::ColorRGBf IMedium::sigma_t() const
{
    return sigma_a() + sigma_s();
}

nmath::scalar_t IMedium::sigma_t_majorant() const
{
    return std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(sigma_t()));
}

nimg::ColorRGBf IMedium::transmittance(const nmath::Vector3f &origin,
                                       const nmath::Vector3f &direction,
                                       nmath::scalar_t dist) const
{
    const nmath::Vector3f p = origin + direction * std::max((nmath::scalar_t)0.0, dist * (nmath::scalar_t)0.5);
    const nimg::ColorRGBf st = sigma_t_at(p);
    const nmath::scalar_t d = std::max((nmath::scalar_t)0.0, dist);
    return nimg::ColorRGBf(
          (nmath::scalar_t)std::exp(-(double)(st.r() * d))
        , (nmath::scalar_t)std::exp(-(double)(st.g() * d))
        , (nmath::scalar_t)std::exp(-(double)(st.b() * d))
    );
}

bool IMedium::sample_distance(const nmath::Vector3f &origin,
                              const nmath::Vector3f &direction,
                              nmath::scalar_t segment_dist,
                              nmath::scalar_t &sampled_dist) const
{
    const nmath::Vector3f p = origin + direction * std::max((nmath::scalar_t)0.0, segment_dist * (nmath::scalar_t)0.5);
    const nmath::scalar_t st = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(sigma_t_at(p)));
    if (st <= (nmath::scalar_t)EPSILON) {
        sampled_dist = segment_dist;
        return false;
    }

    const nmath::scalar_t u = std::max(
        (nmath::scalar_t)1e-6,
        std::min((nmath::scalar_t)0.999999, nmath::prng_c(0.0, 1.0))
    );
    const nmath::scalar_t d = -(nmath::scalar_t)std::log((double)(1.0 - u)) / st;
    if (d < segment_dist) {
        sampled_dist = d;
        return true;
    }

    sampled_dist = segment_dist;
    return false;
}

nimg::ColorRGBf IMedium::scattering_weight(const nmath::Vector3f &p) const
{
    const nmath::scalar_t st = std::max((nmath::scalar_t)0.0, (nmath::scalar_t)nimg::eval::luminance(sigma_t_at(p)));
    if (st <= (nmath::scalar_t)EPSILON) return nimg::ColorRGBf(0.0f, 0.0f, 0.0f);
    return sigma_s_at(p) * ((nmath::scalar_t)1.0 / st);
}

nimg::ColorRGBf IMedium::transmittance(nmath::scalar_t dist) const
{
    return transmittance(nmath::Vector3f(0.0f, 0.0f, 0.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f), dist);
}

bool IMedium::sample_distance(nmath::scalar_t segment_dist, nmath::scalar_t &sampled_dist) const
{
    return sample_distance(nmath::Vector3f(0.0f, 0.0f, 0.0f), nmath::Vector3f(0.0f, 0.0f, 1.0f), segment_dist, sampled_dist);
}

nimg::ColorRGBf IMedium::scattering_weight() const
{
    return scattering_weight(nmath::Vector3f(0.0f, 0.0f, 0.0f));
}

} // namespace medium
} // namespace asset
} // namespace xtcore
