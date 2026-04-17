#ifndef XTCORE_SAMPLER_BLEND_H_INCLUDED
#define XTCORE_SAMPLER_BLEND_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Blend : public ISampler
{
    public:
    Blend();
    virtual ~Blend();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    ISampler *a;
    ISampler *b;
    nmath::scalar_t t;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_BLEND_H_INCLUDED */
