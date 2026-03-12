#ifndef XTCORE_SAMPLER_WEAVE_H_INCLUDED
#define XTCORE_SAMPLER_WEAVE_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Weave : public ISampler
{
    public:
    Weave();
    virtual ~Weave();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf base_color;
    nimg::ColorRGBf warp_color;
    nimg::ColorRGBf weft_color;
    nmath::scalar_t scale;
    nmath::scalar_t band_width;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_WEAVE_H_INCLUDED */
