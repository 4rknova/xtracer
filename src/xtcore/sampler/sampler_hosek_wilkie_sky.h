#ifndef XTCORE_SAMPLER_HOSEK_WILKIE_SKY_H_INCLUDED
#define XTCORE_SAMPLER_HOSEK_WILKIE_SKY_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class HosekWilkieSky : public ISampler
{
    public:
    HosekWilkieSky();
    virtual ~HosekWilkieSky();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nmath::Vector3f sun_direction;
    nmath::scalar_t turbidity;
    nmath::scalar_t ground_albedo;
    nmath::scalar_t exposure;
    nimg::ColorRGBf ground_color;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_HOSEK_WILKIE_SKY_H_INCLUDED */
