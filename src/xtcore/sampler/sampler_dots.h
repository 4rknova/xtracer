#ifndef XTCORE_SAMPLER_DOTS_H_INCLUDED
#define XTCORE_SAMPLER_DOTS_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Dots : public ISampler
{
    public:
    Dots();
    virtual ~Dots();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_bg;
    nimg::ColorRGBf color_dot;
    nmath::scalar_t scale;
    nmath::scalar_t radius;
    nmath::scalar_t softness;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_DOTS_H_INCLUDED */
