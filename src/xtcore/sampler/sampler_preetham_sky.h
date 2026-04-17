#ifndef XTCORE_SAMPLER_PREETHAM_SKY_H_INCLUDED
#define XTCORE_SAMPLER_PREETHAM_SKY_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class PreethamSky : public ISampler
{
    public:
    PreethamSky();
    virtual ~PreethamSky();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nmath::Vector3f sun_direction;
    nmath::scalar_t turbidity;
    nmath::scalar_t exposure;
    nimg::ColorRGBf ground_color;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_PREETHAM_SKY_H_INCLUDED */
