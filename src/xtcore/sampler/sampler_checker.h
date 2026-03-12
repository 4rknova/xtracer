#ifndef XTCORE_SAMPLER_CHECKER_H_INCLUDED
#define XTCORE_SAMPLER_CHECKER_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Checker : public ISampler
{
    public:
    Checker();
    virtual ~Checker();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_a;
    nimg::ColorRGBf color_b;
    nmath::scalar_t scale_u;
    nmath::scalar_t scale_v;
    nmath::scalar_t offset_u;
    nmath::scalar_t offset_v;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_CHECKER_H_INCLUDED */
