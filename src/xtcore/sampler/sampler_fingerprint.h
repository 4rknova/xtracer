#ifndef XTCORE_SAMPLER_FINGERPRINT_H_INCLUDED
#define XTCORE_SAMPLER_FINGERPRINT_H_INCLUDED

#include <xtcore/sampler.h>

namespace xtcore {
    namespace sampler {

class Fingerprint : public ISampler
{
    public:
    Fingerprint();
    virtual ~Fingerprint();

    virtual nimg::ColorRGBf sample(const nmath::Vector3f &uvw) const;

    nimg::ColorRGBf color_a;
    nimg::ColorRGBf color_b;
    nmath::scalar_t scale;
    nmath::scalar_t ridge_frequency;
    nmath::scalar_t ridge_width;
    nmath::scalar_t distortion;
};

    } /* namespace sampler */
} /* namespace xtcore */

#endif /* XTCORE_SAMPLER_FINGERPRINT_H_INCLUDED */
