#include <algorithm>
#include <cmath>

#include <nmath/mutil.h>

#include "sampler_rayleigh_sky.h"

namespace xtcore {
    namespace sampler {

namespace {

static const nmath::scalar_t k_pi = (nmath::scalar_t)3.14159265358979323846;
static const nmath::scalar_t k_deg_to_rad = k_pi / (nmath::scalar_t)180.0;

inline nimg::ColorRGBf exp_color(const nimg::ColorRGBf &c)
{
    return nimg::ColorRGBf(
        (float)std::exp((double)c.r()),
        (float)std::exp((double)c.g()),
        (float)std::exp((double)c.b()));
}

inline nimg::ColorRGBf clamp_min_color(const nimg::ColorRGBf &c, nmath::scalar_t v)
{
    return nimg::ColorRGBf(
        (float)std::max(c.r(), (float)v),
        (float)std::max(c.g(), (float)v),
        (float)std::max(c.b(), (float)v));
}

inline nmath::scalar_t smoothstep(nmath::scalar_t a, nmath::scalar_t b, nmath::scalar_t x)
{
    if (a == b) return x < a ? (nmath::scalar_t)0.0 : (nmath::scalar_t)1.0;
    const nmath::scalar_t t = nmath::saturate((x - a) / (b - a));
    return t * t * ((nmath::scalar_t)3.0 - (nmath::scalar_t)2.0 * t);
}

} // namespace

RayleighSky::RayleighSky()
    : sun_direction((nmath::scalar_t)0.35, (nmath::scalar_t)0.8, (nmath::scalar_t)0.2)
    , sun_intensity(24.0f, 22.0f, 18.0f)
    , beta_rayleigh(0.18f, 0.35f, 0.80f)
    , ground_color(0.02f, 0.02f, 0.03f)
    , density((nmath::scalar_t)1.0)
    , horizon_falloff((nmath::scalar_t)1.5)
    , sun_disk_radius((nmath::scalar_t)1.0)
    , sun_disk_intensity((nmath::scalar_t)1.0)
    , sun_glow_radius((nmath::scalar_t)8.0)
    , sun_glow_intensity((nmath::scalar_t)0.35)
    , sun_glow_falloff((nmath::scalar_t)4.0)
{}

nimg::ColorRGBf RayleighSky::sample(const nmath::Vector3f &tc) const
{
    const nmath::Vector3f dir = tc.normalized();
    nmath::Vector3f sun_dir = sun_direction;
    if (sun_dir.length_squared() <= (nmath::scalar_t)0.0) sun_dir = nmath::Vector3f((nmath::scalar_t)0.0, (nmath::scalar_t)1.0, (nmath::scalar_t)0.0);
    else sun_dir = sun_dir.normalized();

    const nmath::scalar_t mu = nmath::clamp(nmath::dot(dir, sun_dir), (nmath::scalar_t)-1.0, (nmath::scalar_t)1.0);
    const nmath::scalar_t view_up = std::max((nmath::scalar_t)-0.2, dir.y);
    const nmath::scalar_t sun_up = std::max((nmath::scalar_t)-0.2, sun_dir.y);
    const nmath::scalar_t view_airmass = (nmath::scalar_t)1.0 / std::max((nmath::scalar_t)0.12, view_up + (nmath::scalar_t)0.28);
    const nmath::scalar_t sun_airmass = (nmath::scalar_t)1.0 / std::max((nmath::scalar_t)0.12, sun_up + (nmath::scalar_t)0.28);
    const nmath::scalar_t phase = ((nmath::scalar_t)3.0 / ((nmath::scalar_t)16.0 * k_pi)) * ((nmath::scalar_t)1.0 + mu * mu);

    const nimg::ColorRGBf beta = clamp_min_color(beta_rayleigh, (nmath::scalar_t)0.0f) * (float)std::max((nmath::scalar_t)0.0, density);
    const nmath::scalar_t optical_scale = std::max((nmath::scalar_t)0.0, horizon_falloff) * (view_airmass + sun_airmass);
    const nimg::ColorRGBf transmittance = exp_color(beta * (float)(-optical_scale));
    nimg::ColorRGBf sky = (sun_intensity * beta) * (float)(phase * view_airmass);
    sky *= transmittance;

    const nmath::scalar_t sky_visibility = smoothstep((nmath::scalar_t)-0.12, (nmath::scalar_t)0.18, dir.y);
    const nmath::scalar_t ground_visibility = smoothstep((nmath::scalar_t)-0.35, (nmath::scalar_t)0.02, -dir.y);
    sky *= (float)sky_visibility;
    sky = ground_color * (float)ground_visibility + sky;

    const nmath::scalar_t sun_angle = std::acos((double)mu);
    const nmath::scalar_t disk_radius = std::max((nmath::scalar_t)0.001, sun_disk_radius * k_deg_to_rad);
    const nmath::scalar_t glow_radius = std::max((nmath::scalar_t)0.001, sun_glow_radius * k_deg_to_rad);
    const nmath::scalar_t disk = (sun_angle <= disk_radius) ? (nmath::scalar_t)1.0 : (nmath::scalar_t)0.0;
    const nmath::scalar_t glow = std::exp(-(double)((sun_angle / glow_radius) * std::max((nmath::scalar_t)0.0, sun_glow_falloff)));

    const nmath::scalar_t sun_visibility = smoothstep((nmath::scalar_t)-0.08, (nmath::scalar_t)0.12, dir.y);
    sky += sun_intensity * (float)((std::max((nmath::scalar_t)0.0, sun_disk_intensity) * disk
                                  + std::max((nmath::scalar_t)0.0, sun_glow_intensity) * glow) * sun_visibility);
    return sky;
}

    } /* namespace sampler */
} /* namespace xtcore */
