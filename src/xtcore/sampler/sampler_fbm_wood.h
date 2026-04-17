#ifndef XTCORE_SAMPLER_FBM_WOOD_H_INCLUDED
#define XTCORE_SAMPLER_FBM_WOOD_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class FBMWood : public ISampler
{
    public:
    FBMWood();
    virtual ~FBMWood();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_a;
    nimg::ColorRGBf color_b;
    nmath::scalar_t scale;
    nmath::scalar_t ring_frequency;
    nmath::scalar_t turbulence;
    int octaves;
    nmath::scalar_t lacunarity;
    nmath::scalar_t gain;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_FBM_WOOD_H_INCLUDED */
