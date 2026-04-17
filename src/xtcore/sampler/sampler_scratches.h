#ifndef XTCORE_SAMPLER_SCRATCHES_H_INCLUDED
#define XTCORE_SAMPLER_SCRATCHES_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Scratches : public ISampler
{
    public:
    Scratches();
    virtual ~Scratches();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_base;
    nimg::ColorRGBf color_scratch;
    nmath::scalar_t scale;
    int density;
    nmath::scalar_t width;
    nmath::scalar_t angle;
    nmath::scalar_t angle_jitter;
    int seed;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_SCRATCHES_H_INCLUDED */
