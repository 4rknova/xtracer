#ifndef XTCORE_SAMPLER_MIX_MASKED_H_INCLUDED
#define XTCORE_SAMPLER_MIX_MASKED_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class MixMasked : public ISampler
{
    public:
    enum Mode { MODE_LERP = 0, MODE_MULTIPLY, MODE_ADD, MODE_SCREEN };

    MixMasked();
    virtual ~MixMasked();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    ISampler *base;
    ISampler *overlay;
    ISampler *mask;
    nmath::scalar_t t;
    Mode mode;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_MIX_MASKED_H_INCLUDED */
