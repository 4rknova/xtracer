#ifndef XTCORE_SAMPLER_TRIPLANAR_H_INCLUDED
#define XTCORE_SAMPLER_TRIPLANAR_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Triplanar : public ISampler
{
    public:
    Triplanar();
    virtual ~Triplanar();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    ISampler *child;
    nmath::scalar_t scale;
    nmath::scalar_t blend_sharpness;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_TRIPLANAR_H_INCLUDED */
