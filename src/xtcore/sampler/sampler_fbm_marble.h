#ifndef XTCORE_SAMPLER_FBM_MARBLE_H_INCLUDED
#define XTCORE_SAMPLER_FBM_MARBLE_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class FBMMarble : public ISampler
{
    public:
    FBMMarble();
    virtual ~FBMMarble();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_a;
    nimg::ColorRGBf color_b;
    nimg::ColorRGBf vein_color;

    nmath::scalar_t scale;
    nmath::scalar_t vein_frequency;
    nmath::scalar_t turbulence;
    int octaves;
    nmath::scalar_t lacunarity;
    nmath::scalar_t gain;
    nmath::scalar_t vein_strength;
    nmath::scalar_t vein_sharpness;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_FBM_MARBLE_H_INCLUDED */
