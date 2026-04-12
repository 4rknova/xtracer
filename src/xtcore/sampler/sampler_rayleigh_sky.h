#ifndef XTCORE_SAMPLER_RAYLEIGH_SKY_H_INCLUDED
#define XTCORE_SAMPLER_RAYLEIGH_SKY_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class RayleighSky : public ISampler
{
    public:
    RayleighSky();

    nimg::ColorRGBf sample(const nmath::Vector3f &tc) const;

    nmath::Vector3f sun_direction;
    nimg::ColorRGBf sun_intensity;
    nimg::ColorRGBf beta_rayleigh;
    nimg::ColorRGBf ground_color;
    nmath::scalar_t density;
    nmath::scalar_t horizon_falloff;
    nmath::scalar_t sun_disk_radius;
    nmath::scalar_t sun_disk_intensity;
    nmath::scalar_t sun_glow_radius;
    nmath::scalar_t sun_glow_intensity;
    nmath::scalar_t sun_glow_falloff;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_RAYLEIGH_SKY_H_INCLUDED */
